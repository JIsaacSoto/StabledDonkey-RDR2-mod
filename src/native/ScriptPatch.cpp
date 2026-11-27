#include "ScriptPatch.h"

#include "Config.h"
#include "Log.h"
#include "Memory.h"
#include "mount/MountAdapter.h"

#include "natives.h"

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

// Script patch. The decompiled 1491.50 scripts (Halen84/RDR3-Decompiled-Scripts)
// exclude donkeys and mules from horse features with explicit model checks:
//
//   player_horse.c      bool func_392(m) { switch (m) { case A_C_DONKEY_01: return true; } return false; }
//                       bool func_393(m) { switch (m) { case A_C_HORSEMULE_01: case A_C_HORSEMULEPAINTED_01: return true; } ... }
//                       (skip bonding, care and stat events for those animals)
//   shop_horse_shop_sp  if (GET_ENTITY_MODEL(mount) == A_C_DONKEY_01 || ... x3) -> TF_SHOP_HORSE_KICK_OUT_DONKEY
//
// Every other use of the donkey hash (breed name, horse class, the
// HORSE_DONKEY catalog item that saves and restores the stabled animal) must
// stay intact, so only two shapes are rewritten in the loaded bytecode:
//   1. case values of small switch helpers "LOCAL_U8_LOAD 0; SWITCH n" made
//      only of donkey / mule hashes, and
//   2. "PUSH_CONST_U32 <donkey>; IEQ" comparisons that appear in a cluster
//      inside the stable script (the kick-out check compares 3 times).
//
// PC 1491.50 bytecode, read from live memory (opcodes differ from the public
// console tables and SWITCH has a 16-bit case count):
//
//   is-donkey helper   22 01 03 00 00 | 66 00 | 3C 01 00 | 7B 7A A3 69 03 00 | 68 07 00 | 09 50 01 01 ...
//                      ENTER 1 param  | LOCAL_U8_LOAD 0 | SWITCH n=1 | case donkey -> +3 | J | PUSH 1; LEAVE
//   stable kick-out    03 05 02 35 | 37 7B 7A A3 69 | 0B | 69
//                      NATIVE GET_ENTITY_MODEL | PUSH_CONST_U32 donkey | IEQ | IAND
//
// Scripts are located by a string literal unique to each. Scans run on a
// background thread (private memory only; write-combined GPU memory is far
// too slow to read); the patched sites are verified every few seconds and a
// rescan runs whenever the game reloads one of the scripts.
namespace StabledDonkey::ScriptPatch
{
    namespace
    {
        constexpr uint32_t DONKEY = Models::DONKEY;
        constexpr uint32_t MULE = Models::MULE;
        constexpr uint32_t PAINTED_MULE = Models::PAINTED_MULE;
        constexpr uint32_t MARKER_DONKEY = 0x5D0D0001;
        constexpr uint32_t MARKER_MULE = 0x5D0D0002;
        constexpr uint32_t MARKER_PAINTED_MULE = 0x5D0D0003;

        // Stable mule mapping (StableMules=1). The model -> item
        // functions (player_horse func_542, shop_horse_shop_sp func_1318) start
        // with "if (model == A_C_HORSE_MP_MANGY_BACKUP) return
        // HORSE_BREED_MANGY_BACKUP;", a multiplayer-only horse. That prefix is
        // rewritten to "if (model == <mule>) return HORSE_DONKEY;" so a stabled
        // mule gets a valid catalog item (no HORSE_MULE item exists). Only one
        // model fits: the regular mule by default; the painted mule only shows
        // up in one mission, where it has to be handed over.
        constexpr uint32_t MANGY_MODEL = 0x30331B80; // joaat("A_C_HORSE_MP_MANGY_BACKUP")
        constexpr uint32_t MANGY_ITEM = 0x215F7CAE;  // joaat("HORSE_BREED_MANGY_BACKUP")
        constexpr uint32_t DONKEY_ITEM = 0xEF006BE0; // joaat("HORSE_DONKEY")
        constexpr size_t MANGY_RESULT_WINDOW = 16;

        constexpr uint8_t OP_IEQ = 0x0B;
        constexpr uint8_t OP_PUSH_CONST_U32 = 0x37;
        constexpr uint8_t OP_LOCAL_U8_LOAD = 0x66;
        constexpr uint8_t OP_SWITCH = 0x3C;

        constexpr Hash SHOP_HORSE_SCRIPT = 0x44EF5546;   // joaat("shop_horse_shop_sp")
        constexpr Hash PLAYER_HORSE_SCRIPT = 0x07C73408; // joaat("player_horse")

        constexpr const char *MARKERS[] = {
            "DEAD_HORSE_SADDLE_SWAP_LONG_LOOT_ANIM_DATA", // player_horse
            "TF_SHOP_HORSE_KICK_OUT_DONKEY",              // shop_horse_shop_sp
        };
        constexpr size_t MARKER_PLAYER_HORSE = 0;
        constexpr size_t MARKER_SHOP = 1;
        constexpr size_t MARKER_COUNT = sizeof(MARKERS) / sizeof(MARKERS[0]);

        constexpr size_t MAX_HITS = 8192;
        constexpr size_t MAX_MARKER_HITS = 64;
        constexpr size_t MAX_REGIONS = 4096;
        constexpr uintptr_t CLUSTER_BYTES = 256;
        constexpr size_t FAST_SCAN_MAX_REGION_BYTES = 64ull * 1024 * 1024;
        constexpr ULONGLONG FIRST_SCAN_DELAY_MS = 3000;
        constexpr ULONGLONG SCRIPT_START_DELAY_MS = 1500;
        constexpr ULONGLONG VERIFY_INTERVAL_MS = 5000;

        enum HitKind : uint8_t
        {
            Raw = 0,
            SwitchCase = 1,
            Compare = 2
        };

        struct Hit
        {
            uintptr_t address; // address of the 4-byte hash constant
            uint32_t value;
            uint16_t region;
            uint8_t kind;
            uint8_t opcode; // switch: opcode before the count
            uint8_t next;   // switch: load opcode before SWITCH
        };

        struct MarkerHit
        {
            uintptr_t address;
            uint16_t region;
            uint8_t marker;
        };

        struct ScanJob
        {
            std::atomic<int> state{0}; // 0 idle, 1 running, 2 done
            bool includeMules = false;
            bool includeStableMule = false;
            size_t maxRegionBytes = 0; // 0 = scan every region
            bool discard = false;      // set when the mod restarted mid-scan
            const char *reason = "";
            Hit hits[MAX_HITS];
            size_t hitCount = 0;
            bool overflow = false;
            MarkerHit markers[MAX_MARKER_HITS];
            size_t markerCount = 0;
            size_t regionCount = 0;
            unsigned long long bytesScanned = 0;
            unsigned faults = 0;
            ULONGLONG elapsedMs = 0;
            uint16_t currentRegion = 0xFFFF;
        };

        ScanJob g_job;

        struct PatchedSite
        {
            uintptr_t address;
            uint32_t marker;
        };
        std::vector<PatchedSite> g_patched;
        ULONGLONG g_nextVerifyAt = 0;
        ULONGLONG g_nextScanAt = 0;
        const char *g_nextScanReason = "startup";
        bool g_nextScanFull = false;
        int g_lastShopInstances = -1;
        int g_lastPlayerHorseInstances = -1;

        bool IsFamily(uint32_t value)
        {
            return value == DONKEY || value == MULE || value == PAINTED_MULE || value == MARKER_DONKEY ||
                   value == MARKER_MULE || value == MARKER_PAINTED_MULE;
        }

        uint32_t LoadU32(const uint8_t *p)
        {
            uint32_t value = 0;
            std::memcpy(&value, p, sizeof(value));
            return value;
        }

        // Regions get an index the first time they produce a hit, so the
        // stable-script region can be recognised later.
        uint16_t CurrentRegion(ScanJob &job)
        {
            if (job.currentRegion != 0xFFFF)
                return job.currentRegion;
            if (job.regionCount >= MAX_REGIONS)
                return 0xFFFF;
            job.currentRegion = static_cast<uint16_t>(job.regionCount++);
            return job.currentRegion;
        }

        void ScanHashes(ScanJob &job, const uint8_t *begin, const uint8_t *end, uint32_t needle)
        {
            // Anchor on the third byte, rarer in memory than the first ones.
            const uint8_t anchor = static_cast<uint8_t>((needle >> 16) & 0xFF);
            const uint8_t *p = begin + 2;
            while (end - p >= 2)
            {
                p = static_cast<const uint8_t *>(std::memchr(p, anchor, static_cast<size_t>(end - p - 1)));
                if (!p)
                    break;
                const uint8_t *value = p - 2;
                ++p;
                if (LoadU32(value) != needle)
                    continue;
                if (job.hitCount >= MAX_HITS)
                {
                    job.overflow = true;
                    return;
                }

                Hit hit{reinterpret_cast<uintptr_t>(value), needle, CurrentRegion(job), Raw, 0, 0};

                // Switch layout: [op][u16 count] then count x ([u32 value][s16 jump]).
                for (int k = 0; k < 3 && hit.kind == Raw; ++k)
                {
                    const uint8_t *op = value - 3 - 6 * k;
                    if (op < begin)
                        break;
                    const uint16_t count = static_cast<uint16_t>(op[1] | (op[2] << 8));
                    if (count <= k || count > 3 || op + 3 + 6 * count > end)
                        continue;
                    bool onlyFamily = true;
                    for (int e = 0; e < count && onlyFamily; ++e)
                        onlyFamily = IsFamily(LoadU32(op + 3 + 6 * e));
                    if (!onlyFamily)
                        continue;
                    hit.kind = SwitchCase;
                    hit.opcode = op[0];
                    hit.next = (op - 2 >= begin && op[-1] == 0) ? op[-2] : 0xFF;
                }

                if (hit.kind == Raw && value > begin && end - value >= 5 && value[-1] == OP_PUSH_CONST_U32 &&
                    value[4] == OP_IEQ)
                    hit.kind = Compare;
                job.hits[job.hitCount++] = hit;
            }
        }

        void ScanMarkers(ScanJob &job, const uint8_t *begin, const uint8_t *end)
        {
            for (size_t m = 0; m < MARKER_COUNT; ++m)
            {
                const auto *pattern = reinterpret_cast<const uint8_t *>(MARKERS[m]);
                const size_t length = std::strlen(MARKERS[m]) + 1; // include terminator
                const uint8_t *p = begin;
                while (end - p >= static_cast<ptrdiff_t>(length))
                {
                    p = std::search(p, end, std::boyer_moore_horspool_searcher(pattern, pattern + length));
                    if (p == end)
                        break;
                    if (job.markerCount < MAX_MARKER_HITS)
                        job.markers[job.markerCount++] = {reinterpret_cast<uintptr_t>(p), CurrentRegion(job),
                                                          static_cast<uint8_t>(m)};
                    p += length;
                }
            }
        }

        bool ScanRegionSafe(ScanJob &job, const MEMORY_BASIC_INFORMATION &info)
        {
            const auto *base = reinterpret_cast<const uint8_t *>(info.BaseAddress);
            const uint8_t *end = base + info.RegionSize;
            __try
            {
                ScanHashes(job, base, end, DONKEY);
                if (job.includeMules)
                {
                    ScanHashes(job, base, end, MULE);
                    ScanHashes(job, base, end, PAINTED_MULE);
                }
                if (job.includeStableMule)
                    ScanHashes(job, base, end, MANGY_MODEL);
                ScanMarkers(job, base, end);
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        bool IsScannable(const MEMORY_BASIC_INFORMATION &info)
        {
            // Script programs live in the game's private heap; mapped and
            // write-combined (GPU) memory is skipped.
            if (info.State != MEM_COMMIT || info.Type != MEM_PRIVATE ||
                (info.Protect & (PAGE_GUARD | PAGE_NOACCESS | PAGE_NOCACHE | PAGE_WRITECOMBINE)) != 0)
                return false;
            const DWORD protect = info.Protect & 0xFF;
            return protect == PAGE_READWRITE || protect == PAGE_READONLY || protect == PAGE_WRITECOPY ||
                   protect == PAGE_EXECUTE_READ || protect == PAGE_EXECUTE_READWRITE ||
                   protect == PAGE_EXECUTE_WRITECOPY;
        }

        DWORD WINAPI ScanThread(LPVOID)
        {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
            ScanJob &job = g_job;
            const ULONGLONG start = GetTickCount64();
            const uintptr_t jobAddress = reinterpret_cast<uintptr_t>(&job);
            uintptr_t address = 0x10000;
            MEMORY_BASIC_INFORMATION info{};
            while (VirtualQuery(reinterpret_cast<const void *>(address), &info, sizeof(info)) != 0)
            {
                const uintptr_t base = reinterpret_cast<uintptr_t>(info.BaseAddress);
                const size_t size = info.RegionSize;
                const bool containsJob = jobAddress >= base && jobAddress < base + size;
                // Fast pass: scripts sit in small regions; huge heaps hold data.
                const bool tooLarge = job.maxRegionBytes != 0 && size > job.maxRegionBytes;
                if (IsScannable(info) && !containsJob && !tooLarge)
                {
                    job.currentRegion = 0xFFFF;
                    if (!ScanRegionSafe(job, info))
                        ++job.faults;
                    job.bytesScanned += size;
                }
                const uintptr_t next = base + size;
                if (next <= address)
                    break;
                address = next;
            }
            job.elapsedMs = GetTickCount64() - start;
            job.state.store(2, std::memory_order_release);
            return 0;
        }

        void StartScan(const char *reason, bool fullScan)
        {
            ScanJob &job = g_job;
            job.maxRegionBytes = fullScan ? 0 : FAST_SCAN_MAX_REGION_BYTES;
            job.discard = false;
            job.includeMules = g_config.patchMules;
            job.includeStableMule = g_config.patchMules && g_config.stableMules;
            job.reason = reason;
            job.hitCount = 0;
            job.markerCount = 0;
            job.regionCount = 0;
            job.overflow = false;
            job.bytesScanned = 0;
            job.faults = 0;
            job.elapsedMs = 0;
            job.state.store(1, std::memory_order_release);
            HANDLE thread = CreateThread(nullptr, 0, ScanThread, nullptr, 0, nullptr);
            if (!thread)
            {
                Log("Script patch: could not start the scan thread (error %lu)", GetLastError());
                job.state.store(0, std::memory_order_release);
                return;
            }
            CloseHandle(thread);
        }

        uint32_t MarkerFor(uint32_t value)
        {
            switch (value)
            {
            case DONKEY:
                return MARKER_DONKEY;
            case MULE:
                return MARKER_MULE;
            default:
                return MARKER_PAINTED_MULE;
            }
        }

        bool PatchHit(const Hit &hit)
        {
            if (!Memory::IsReadable(hit.address, 4))
                return false;
            uint32_t current = 0;
            std::memcpy(&current, reinterpret_cast<const void *>(hit.address), 4);
            if (current != hit.value)
                return false;
            const uint32_t marker = MarkerFor(hit.value);
            return Memory::WriteBytes(hit.address, &marker, sizeof(marker));
        }

        bool IsPatchedSite(uintptr_t address)
        {
            for (const PatchedSite &site : g_patched)
                if (site.address == address)
                    return true;
            return false;
        }

        // Exact shape of the model -> item prefix, read from live bytecode
        // (2026-11-26):
        //   37 <MANGY_MODEL> 15 xx xx 37 <MANGY_ITEM> 50 01 01
        //   PUSH_CONST_U32 model | fused compare-and-jump | PUSH_CONST_U32 item | LEAVE 1 1
        // A looser match (item anywhere within 16 bytes) also hit a 7th, unknown
        // site, and the 0.4.0 test then crashed when feeding a wild mule from
        // the TAB menu; anything that does not fit exactly is now left intact.
        constexpr uint8_t OP_FUSED_EQ_JUMP = 0x15;
        constexpr uint8_t OP_LEAVE = 0x50;

        void LogBytes(const char *what, uintptr_t at)
        {
            if (!Memory::IsReadable(at - 8, 32))
                return;
            char bytes[3 * 32 + 1]{};
            size_t used = 0;
            for (uintptr_t a = at - 8; a < at + 24; ++a)
                used += std::snprintf(bytes + used, sizeof(bytes) - used, "%02X ", *reinterpret_cast<const uint8_t *>(a));
            Log("Script patch: %s at %p: %s", what, reinterpret_cast<void *>(at), bytes);
        }

        // Both mules (v0.5). The 18-byte prefix of the model -> item function
        //   66 00 | 37 <MANGY_MODEL> | 15 08 00 | 37 <MANGY_ITEM> | 50 01 01
        //   LOCAL_U8_LOAD 0 | PUSH model | if != jump +8 | PUSH item | LEAVE
        // becomes a two-case switch into the function's own
        // "case A_C_DONKEY_01: return HORSE_DONKEY;" body:
        //   66 00 | 3C 02 00 | <MULE> <jump> | <PAINTED_MULE> <jump> | NOP
        // Every other model falls through to the original switch exactly as
        // before (only the multiplayer-only mangy horse loses its case, as in
        // v0.4). Unlike sending the switch default to HORSE_DONKEY, this keeps
        // the gang horses (Dutch's, Bill's...) and the other models without an
        // item out of the stable, as in vanilla.
        // PC opcode table: maybegreat48/GTA-V-Script-Decompiler Instruction.cs
        // (ShuffledOpcodes, from EROOTIIK's RDR 1355 set); it matches every
        // opcode above that was read from live memory.
        constexpr uint8_t OP_NOP = 0x13;
        constexpr size_t PREFIX_BYTES = 18;
        constexpr size_t PREFIX_LOAD_BYTES = 2; // "66 00" before the model constant
        constexpr uint16_t MIN_ITEM_CASES = 16;
        constexpr uint16_t MAX_ITEM_CASES = 512;

        // How a SWITCH case offset is counted: from the end of the whole
        // table or from the end of its own entry. Read from the big switch
        // itself (only one of them lands every case on a "return item" body).
        enum class SwitchBase
        {
            Unknown,
            TableEnd,
            EntryEnd
        };
        SwitchBase g_switchBase = SwitchBase::Unknown;

        int16_t LoadS16(const uint8_t *p)
        {
            int16_t value = 0;
            std::memcpy(&value, p, sizeof(value));
            return value;
        }

        const uint8_t *CaseTarget(const uint8_t *table, uint16_t count, uint16_t index, SwitchBase base)
        {
            const uint8_t *entry = table + 6 * index;
            const uint8_t *from = base == SwitchBase::TableEnd ? table + 6 * count : entry + 6;
            return from + LoadS16(entry + 4);
        }

        // "PUSH_CONST_U32 <item>; LEAVE 1 1"
        bool IsReturnConst(const uint8_t *body)
        {
            return Memory::IsReadable(reinterpret_cast<uintptr_t>(body), 8) && body[0] == OP_PUSH_CONST_U32 &&
                   body[5] == OP_LEAVE && body[6] == 1 && body[7] == 1;
        }

        // The model -> item switch that follows the prefix: "66 00 3C <count>"
        // and a table whose every case returns a constant. Returns the donkey
        // case body ("PUSH HORSE_DONKEY; LEAVE"), or null if anything differs.
        constexpr uint16_t MAX_ODD_CASES = 2;

        struct OddCases
        {
            uint16_t count = 0;
            const uint8_t *firstTarget = nullptr;
            std::string detail;
        };

        // why (optional) receives the reason when it returns null; oddInfo
        // (optional) the cases that did not read as "return const".
        const uint8_t *FindDonkeyItemBody(const uint8_t *sw, SwitchBase &baseOut, std::string *why = nullptr,
                                          OddCases *oddInfo = nullptr)
        {
            auto fail = [why](const std::string &reason) -> const uint8_t * {
                if (why)
                    *why = reason;
                return nullptr;
            };
            if (!Memory::IsReadable(reinterpret_cast<uintptr_t>(sw), 5) || sw[0] != OP_LOCAL_U8_LOAD || sw[1] != 0 ||
                sw[2] != OP_SWITCH)
                return fail("no switch after the prefix");
            const uint16_t count = static_cast<uint16_t>(sw[3] | (sw[4] << 8));
            const uint8_t *table = sw + 5;
            if (count < MIN_ITEM_CASES || count > MAX_ITEM_CASES)
                return fail("case count " + std::to_string(count));
            if (!Memory::IsReadable(reinterpret_cast<uintptr_t>(table), 6u * count))
                return fail("table not readable (count " + std::to_string(count) + ")");

            // A jump base fits when (nearly) every case lands on "return const".
            // A couple of odd cases are tolerated (seen once: one copy where
            // case 80 of 81 has another shape); only the donkey case is used
            // and it is checked exactly below.
            SwitchBase base = SwitchBase::Unknown;
            std::string misses;
            uint16_t baseOdd = 0;
            const uint8_t *firstOdd = nullptr;
            for (SwitchBase candidate : {SwitchBase::TableEnd, SwitchBase::EntryEnd})
            {
                uint16_t odd = 0;
                int first = -1;
                for (uint16_t i = 0; i < count && odd <= MAX_ODD_CASES; ++i)
                    if (!IsReturnConst(CaseTarget(table, count, i, candidate)))
                    {
                        if (first < 0)
                            first = i;
                        ++odd;
                    }
                if (first >= 0)
                {
                    char buf[112];
                    std::snprintf(buf, sizeof(buf), "%s%s: %s%u odd, first case %d of %u (value %08X jump %d)",
                                  misses.empty() ? "" : ", ",
                                  candidate == SwitchBase::TableEnd ? "table end" : "entry end",
                                  odd > MAX_ODD_CASES ? ">" : "", odd > MAX_ODD_CASES ? MAX_ODD_CASES : odd, first, count,
                                  LoadU32(table + 6 * first), LoadS16(table + 6 * first + 4));
                    misses += buf;
                }
                if (odd > MAX_ODD_CASES)
                    continue;
                if (base != SwitchBase::Unknown)
                    return fail("both jump bases fit");
                base = candidate;
                baseOdd = odd;
                firstOdd = first >= 0 ? CaseTarget(table, count, static_cast<uint16_t>(first), candidate) : nullptr;
            }
            if (base == SwitchBase::Unknown)
                return fail("cases do not return a constant: " + misses);
            if (oddInfo)
            {
                oddInfo->count = baseOdd;
                oddInfo->firstTarget = firstOdd;
                oddInfo->detail = misses;
            }

            for (uint16_t i = 0; i < count; ++i)
            {
                if (LoadU32(table + 6 * i) != DONKEY)
                    continue;
                const uint8_t *body = CaseTarget(table, count, i, base);
                if (!IsReturnConst(body) || LoadU32(body + 1) != DONKEY_ITEM)
                    return fail("donkey case does not return HORSE_DONKEY");
                baseOut = base;
                return body;
            }
            return fail("no donkey case");
        }

        // Case values must stay sorted (signed): the mule hash is negative.
        constexpr uint32_t SMALL_CASES[2] = {MULE, PAINTED_MULE};
        static_assert(static_cast<int32_t>(MULE) < static_cast<int32_t>(PAINTED_MULE), "case order");

        // Bytes written over the prefix after its "66 00".
        bool BuildMuleSwitch(const uint8_t *prefix, const uint8_t *donkeyBody, SwitchBase base,
                             uint8_t (&out)[PREFIX_BYTES - PREFIX_LOAD_BYTES])
        {
            const uint8_t *table = prefix + PREFIX_LOAD_BYTES + 3;
            out[0] = OP_SWITCH;
            out[1] = 2;
            out[2] = 0;
            for (uint16_t i = 0; i < 2; ++i)
            {
                const uint8_t *from = base == SwitchBase::TableEnd ? table + 12 : table + 6 * (i + 1);
                const ptrdiff_t offset = donkeyBody - from;
                if (offset < INT16_MIN || offset > INT16_MAX)
                    return false;
                const int16_t jump = static_cast<int16_t>(offset);
                std::memcpy(out + 3 + 6 * i, &SMALL_CASES[i], 4);
                std::memcpy(out + 3 + 6 * i + 4, &jump, 2);
            }
            out[15] = OP_NOP;
            return true;
        }

        // Our switch, already in place (mod restarted without a script reload).
        bool IsMuleSwitch(const uint8_t *op)
        {
            if (!Memory::IsReadable(reinterpret_cast<uintptr_t>(op) - PREFIX_LOAD_BYTES, PREFIX_BYTES + 5) ||
                op[-2] != OP_LOCAL_U8_LOAD || op[-1] != 0 || op[0] != OP_SWITCH || op[1] != 2 || op[2] != 0 ||
                LoadU32(op + 3) != MULE || LoadU32(op + 9) != PAINTED_MULE || op[15] != OP_NOP)
                return false;
            SwitchBase base = SwitchBase::Unknown;
            const uint8_t *donkeyBody = FindDonkeyItemBody(op + 16, base);
            return donkeyBody && CaseTarget(op + 3, 2, 0, base) == donkeyBody &&
                   CaseTarget(op + 3, 2, 1, base) == donkeyBody;
        }

        void TrackMuleSwitch(const uint8_t *op)
        {
            g_patched.push_back({reinterpret_cast<uintptr_t>(op + 3), MULE});
            g_patched.push_back({reinterpret_cast<uintptr_t>(op + 9), PAINTED_MULE});
        }

        struct MuleMappingResult
        {
            unsigned bothMules = 0;
            unsigned muleOnly = 0;
            unsigned alreadyApplied = 0;
        };

        // Rewrites the exact mangy prefix of the model -> item functions. The
        // other functions with the same prefix (breed name, class) return
        // something else and are left alone. Where the following switch does
        // not read as expected, falls back to the v0.4 rewrite (regular mule
        // only): "PUSH <mule>; ==; PUSH HORSE_DONKEY; LEAVE".
        MuleMappingResult PatchStableMuleMapping(const std::vector<Hit> &hits)
        {
            MuleMappingResult result;
            for (const Hit &hit : hits)
            {
                if (hit.value != MULE || IsPatchedSite(hit.address))
                    continue;
                const auto *op = reinterpret_cast<const uint8_t *>(hit.address) - 3;
                if (IsMuleSwitch(op))
                {
                    TrackMuleSwitch(op);
                    ++result.alreadyApplied;
                }
            }

            for (const Hit &hit : hits)
            {
                if (hit.value != MANGY_MODEL || !Memory::IsReadable(hit.address - 3, PREFIX_BYTES + 5))
                    continue;
                const auto *p = reinterpret_cast<const uint8_t *>(hit.address);
                const bool exact = p[-1] == OP_PUSH_CONST_U32 && p[4] == OP_FUSED_EQ_JUMP &&
                                   p[7] == OP_PUSH_CONST_U32 && p[12] == OP_LEAVE;
                if (!exact)
                {
                    // Log what the old, looser match would also have patched.
                    if (p[-1] == OP_PUSH_CONST_U32 && Memory::IsReadable(hit.address + 4, MANGY_RESULT_WINDOW + 5))
                        for (uintptr_t a = hit.address + 4; a < hit.address + 4 + MANGY_RESULT_WINDOW; ++a)
                            if (reinterpret_cast<const uint8_t *>(a)[0] == OP_PUSH_CONST_U32 &&
                                LoadU32(reinterpret_cast<const uint8_t *>(a) + 1) == MANGY_ITEM)
                            {
                                LogBytes("mangy site left intact (unexpected shape)", hit.address);
                                break;
                            }
                    continue;
                }
                if (LoadU32(p + 8) != MANGY_ITEM)
                {
                    // Same prefix returning something else (breed name, class).
                    continue;
                }

                const uint8_t *prefix = p - 3;
                const bool loadsParam = prefix[0] == OP_LOCAL_U8_LOAD && prefix[1] == 0 && LoadS16(p + 5) == 8;
                SwitchBase base = SwitchBase::Unknown;
                std::string why = "prefix does not load the model parameter";
                OddCases odd;
                const uint8_t *donkeyBody =
                    loadsParam ? FindDonkeyItemBody(prefix + PREFIX_BYTES, base, &why, &odd) : nullptr;
                if (donkeyBody && odd.count)
                {
                    Log("Script patch: model -> item switch with %u odd case(s), donkey case exact (%s)", odd.count,
                        odd.detail.c_str());
                    if (odd.firstTarget)
                        LogBytes("odd case target", reinterpret_cast<uintptr_t>(odd.firstTarget));
                }
                uint8_t bytes[PREFIX_BYTES - PREFIX_LOAD_BYTES]{};
                if (donkeyBody && !BuildMuleSwitch(prefix, donkeyBody, base, bytes))
                {
                    why = "donkey case too far for a 16-bit jump";
                    donkeyBody = nullptr;
                }
                if (donkeyBody)
                {
                    if (g_switchBase != base)
                    {
                        g_switchBase = base;
                        Log("Script patch: switch jumps count from the %s",
                            base == SwitchBase::TableEnd ? "table end" : "entry end");
                    }
                    const uintptr_t at = reinterpret_cast<uintptr_t>(prefix + PREFIX_LOAD_BYTES);
                    if (Memory::WriteBytes(at, bytes, sizeof(bytes)))
                    {
                        TrackMuleSwitch(prefix + PREFIX_LOAD_BYTES);
                        LogBytes("mule switch written", at);
                        ++result.bothMules;
                    }
                    continue;
                }

                Log("Script patch: model -> item switch not recognised (%s), regular mule only", why.c_str());
                LogBytes("switch bytes", reinterpret_cast<uintptr_t>(prefix + PREFIX_BYTES));
                LogBytes("model -> item site", hit.address);
                const uintptr_t item = hit.address + 8;
                if (!Memory::WriteBytes(item, &DONKEY_ITEM, sizeof(DONKEY_ITEM)))
                    continue;
                if (!Memory::WriteBytes(hit.address, &MULE, sizeof(MULE)))
                    continue;
                g_patched.push_back({hit.address, MULE});
                g_patched.push_back({item, DONKEY_ITEM});
                ++result.muleOnly;
            }
            return result;
        }

        void ApplyResults()
        {
            ScanJob &job = g_job;
            const std::vector<Hit> hits(job.hits, job.hits + job.hitCount);

            // The switch shape confirms that the opcode table is the expected
            // one before any comparison is touched.
            bool opcodesConfirmed = false;
            for (const Hit &hit : hits)
                opcodesConfirmed = opcodesConfirmed ||
                                   (hit.kind == SwitchCase && hit.opcode == OP_SWITCH && hit.next == OP_LOCAL_U8_LOAD);

            auto inShopScript = [&](uint16_t region) {
                for (size_t i = 0; i < job.markerCount; ++i)
                    if (job.markers[i].marker == MARKER_SHOP && job.markers[i].region == region)
                        return true;
                return false;
            };

            std::vector<uintptr_t> compares;
            for (const Hit &hit : hits)
                if (hit.kind == Compare)
                    compares.push_back(hit.address);
            std::sort(compares.begin(), compares.end());
            auto inCluster = [&](uintptr_t address) {
                auto it = std::lower_bound(compares.begin(), compares.end(), address);
                const bool before = it != compares.begin() && address - *(it - 1) <= CLUSTER_BYTES;
                const bool after = it + 1 < compares.end() && *(it + 1) - address <= CLUSTER_BYTES;
                return before || after;
            };

            // First: the main loop below must see our mule switch (a small
            // "mule / painted mule" switch) as patched and leave it alone.
            const MuleMappingResult stableMule =
                job.includeStableMule ? PatchStableMuleMapping(hits) : MuleMappingResult{};

            unsigned patchedSwitch = 0, patchedCompare = 0, patchedMule = 0, muleKeptInStable = 0;
            for (const Hit &hit : hits)
            {
                if (hit.value == MANGY_MODEL || IsPatchedSite(hit.address))
                    continue; // mangy prefix: PatchStableMuleMapping; or ours already
                const bool isDonkey = hit.value == DONKEY;
                const bool allowed = isDonkey ? g_config.patchDonkey : g_config.patchMules;
                bool shouldPatch = false;
                if (hit.kind == SwitchCase)
                    shouldPatch = hit.opcode == OP_SWITCH && hit.next == OP_LOCAL_U8_LOAD;
                else if (hit.kind == Compare)
                    shouldPatch = opcodesConfirmed && inCluster(hit.address) && inShopScript(hit.region);

                // Mules have no catalog item (HORSE_MULE does not exist), so a
                // stabled mule is saved broken. Keep the stable script's mule
                // helper (shop_horse_shop_sp func_2186) intact: the stable then
                // treats mules as vanilla does. Test 2026-11-25 showed that
                // helper sits in the same region as the stable marker string.
                if (allowed && shouldPatch && !isDonkey && inShopScript(hit.region))
                {
                    ++muleKeptInStable;
                    continue;
                }
                if (allowed && shouldPatch && PatchHit(hit))
                {
                    (hit.kind == SwitchCase ? patchedSwitch : patchedCompare)++;
                    if (!isDonkey)
                        ++patchedMule;
                    g_patched.push_back({hit.address, MarkerFor(hit.value)});
                }
            }

            bool foundPlayerHorse = false;
            for (size_t i = 0; i < job.markerCount; ++i)
                foundPlayerHorse = foundPlayerHorse || job.markers[i].marker == MARKER_PLAYER_HORSE;
            if (job.includeStableMule)
                Log("Script patch: stable mule mapping both_mules=%u mule_only=%u already=%u", stableMule.bothMules,
                    stableMule.muleOnly, stableMule.alreadyApplied);

            Log("Script patch: scan (%s) %llu MB in %llu ms, scripts_found=%zu, patched switch=%u compare=%u (mule=%u, mule kept in stable=%u)%s%s",
                job.reason, job.bytesScanned / (1024ull * 1024ull), static_cast<unsigned long long>(job.elapsedMs),
                job.markerCount, patchedSwitch, patchedCompare, patchedMule, muleKeptInStable,
                job.overflow ? " (hit overflow)" : "",
                opcodesConfirmed ? "" : " (opcodes not confirmed: compares left intact)");

            // The fast pass skips large regions. If it did not even find
            // player_horse, scan everything once.
            if (job.maxRegionBytes != 0 && !foundPlayerHorse)
            {
                Log("Script patch: player_horse not found by the fast scan; scheduling a full scan");
                g_nextScanAt = GetTickCount64();
                g_nextScanReason = "full";
                g_nextScanFull = true;
            }
        }

        void ScheduleScanOnScriptStart(Hash script, int &lastInstances, const char *reason, ULONGLONG now)
        {
            const int instances = SCRIPT::_GET_NUMBER_OF_INSTANCES_OF_SCRIPT_WITH_NAME_HASH(script);
            if (lastInstances == 0 && instances > 0)
            {
                g_nextScanAt = std::min(g_nextScanAt, now + SCRIPT_START_DELAY_MS);
                g_nextScanReason = reason;
            }
            lastInstances = instances;
        }

        void VerifyPatchedSites(ULONGLONG now)
        {
            if (g_patched.empty() || now < g_nextVerifyAt || g_job.state.load() != 0)
                return;
            g_nextVerifyAt = now + VERIFY_INTERVAL_MS;
            size_t lost = 0;
            for (const PatchedSite &site : g_patched)
            {
                uint32_t current = 0;
                if (Memory::IsReadable(site.address, 4))
                    std::memcpy(&current, reinterpret_cast<const void *>(site.address), 4);
                if (current != site.marker)
                    ++lost;
            }
            if (lost > 0)
            {
                Log("Script patch: %zu of %zu patched sites changed (script reloaded); rescanning", lost,
                    g_patched.size());
                g_patched.clear();
                g_nextScanAt = now;
                g_nextScanReason = "script_reloaded";
            }
        }
    }

    void Reset()
    {
        // ScriptHook restarts the mod's script when a save is loaded, and the
        // game reloads its scripts with the original checks: forget the old
        // patch sites and scan again shortly.
        if (g_job.state.load(std::memory_order_acquire) == 1)
            g_job.discard = true;
        g_patched.clear();
        g_lastShopInstances = -1;
        g_lastPlayerHorseInstances = -1;
        g_nextScanAt = GetTickCount64() + FIRST_SCAN_DELAY_MS;
        g_nextScanReason = "session_start";
        g_nextScanFull = false;
    }

    void Update()
    {
        if (!g_config.patchDonkey && !g_config.patchMules)
            return;

        const ULONGLONG now = GetTickCount64();
        if (g_nextScanAt == 0)
            g_nextScanAt = now + FIRST_SCAN_DELAY_MS;

        // A script that is unloaded and loaded again (stable visit, mission
        // or chapter change) comes back with the original checks.
        ScheduleScanOnScriptStart(SHOP_HORSE_SCRIPT, g_lastShopInstances, "stable_script_started", now);
        ScheduleScanOnScriptStart(PLAYER_HORSE_SCRIPT, g_lastPlayerHorseInstances, "player_horse_started", now);
        VerifyPatchedSites(now);

        const int state = g_job.state.load(std::memory_order_acquire);
        if (state == 2)
        {
            if (!g_job.discard)
                ApplyResults();
            g_job.state.store(0, std::memory_order_release);
        }
        else if (state == 0 && now >= g_nextScanAt)
        {
            StartScan(g_nextScanReason, g_nextScanFull);
            g_nextScanAt = ~0ull; // next: session start, script reload or patch lost
            g_nextScanReason = "none";
            g_nextScanFull = false;
        }
    }
}
