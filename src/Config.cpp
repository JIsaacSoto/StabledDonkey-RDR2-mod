#include "Config.h"

#include "Log.h"

#include <cstdlib>
#include <string>

namespace StabledDonkey
{
    Config g_config;

    namespace
    {
        constexpr const char *SECTION = "StabledDonkey";
        std::string g_iniPath;
        bool g_iniFound = false;

        bool ReadBool(const char *key, bool fallback)
        {
            return GetPrivateProfileIntA(SECTION, key, fallback ? 1 : 0, g_iniPath.c_str()) != 0;
        }

        int ReadInt(const char *key, int fallback)
        {
            return static_cast<int>(GetPrivateProfileIntA(SECTION, key, fallback, g_iniPath.c_str()));
        }

        // Missing key keeps the default; an empty value is kept as empty.
        void ReadString(const char *key, std::string &value)
        {
            constexpr const char *MISSING = "\x01";
            char buffer[256]{};
            GetPrivateProfileStringA(SECTION, key, MISSING, buffer, sizeof(buffer), g_iniPath.c_str());
            if (std::string(buffer) != MISSING)
                value = buffer;
        }

        void ReadFloat(const char *key, float &value)
        {
            std::string text;
            ReadString(key, text);
            if (!text.empty())
                value = static_cast<float>(std::atof(text.c_str()));
        }
    }

    void Config::Load(HMODULE module)
    {
        // GetPrivateProfile* looks in the Windows directory for relative
        // paths, so always use the absolute path beside the .asi.
        char modulePath[MAX_PATH]{};
        GetModuleFileNameA(module, modulePath, MAX_PATH);
        std::string directory(modulePath);
        const size_t slash = directory.find_last_of("\\/");
        directory = slash == std::string::npos ? "." : directory.substr(0, slash);
        g_iniPath = directory + "\\StabledDonkey.ini";
        g_iniFound = GetFileAttributesA(g_iniPath.c_str()) != INVALID_FILE_ATTRIBUTES;

        patchDonkey = ReadBool("PatchDonkey", patchDonkey);
        patchMules = ReadBool("PatchMules", patchMules);
        stableMules = ReadBool("StableMules", stableMules);
        carePrompts = ReadBool("CarePrompts", carePrompts);
        careMuleOnFoot = ReadBool("CareMuleOnFoot", careMuleOnFoot);
        ReadString("CareInputPat", careInputPat);
        ReadString("CareInputBrush", careInputBrush);
        ReadString("CareInputFeed", careInputFeed);
        ReadString("CareInputMountedBrush", careInputMountedBrush);
        ReadFloat("CareMaxDistance", careMaxDistance);
        careMountedBrushMinDirt = ReadInt("CareMountedBrushMinDirt", careMountedBrushMinDirt);
        ReadString("HideVanillaPromptTypes", hideVanillaPromptTypes);
        ReadString("HideVanillaPromptTypesMule", hideVanillaPromptTypesMule);
        carePromptPriority = ReadInt("CarePromptPriority", carePromptPriority);
        superDonkMode = ReadBool("SuperDonkMode", superDonkMode);
        superDonkHealth = ReadInt("SuperDonkHealth", superDonkHealth);
        superDonkStamina = ReadInt("SuperDonkStamina", superDonkStamina);
        superDonkSpeed = ReadInt("SuperDonkSpeed", superDonkSpeed);
        superDonkAcceleration = ReadInt("SuperDonkAcceleration", superDonkAcceleration);
        saddlePrompts = ReadBool("SaddlePrompts", saddlePrompts);
        ReadString("SaddleInput", saddleInput);
        saddleTempHorse = ReadBool("SaddleTempHorse", saddleTempHorse);
    }

    void Config::LogValues() const
    {
        Log("Config: ini=%s found=%d PatchDonkey=%d PatchMules=%d CarePrompts=%d CareMuleOnFoot=%d StableMules=%d",
            g_iniPath.c_str(), g_iniFound ? 1 : 0, patchDonkey ? 1 : 0, patchMules ? 1 : 0, carePrompts ? 1 : 0,
            careMuleOnFoot ? 1 : 0, stableMules ? 1 : 0);
        Log("Config: CareInputPat=%s CareInputBrush=%s CareInputFeed=%s CareInputMountedBrush=%s CareMaxDistance=%.2f CareMountedBrushMinDirt=%d HideVanillaPromptTypes=%s HideVanillaPromptTypesMule=%s CarePromptPriority=%d",
            careInputPat.c_str(), careInputBrush.c_str(), careInputFeed.c_str(), careInputMountedBrush.c_str(),
            careMaxDistance, careMountedBrushMinDirt, hideVanillaPromptTypes.c_str(),
            hideVanillaPromptTypesMule.c_str(), carePromptPriority);
        Log("Config: SuperDonkMode=%d SuperDonkHealth=%d SuperDonkStamina=%d SuperDonkSpeed=%d SuperDonkAcceleration=%d",
            superDonkMode ? 1 : 0, superDonkHealth, superDonkStamina, superDonkSpeed, superDonkAcceleration);
        Log("Config: SaddlePrompts=%d SaddleInput=%s SaddleTempHorse=%d", saddlePrompts ? 1 : 0, saddleInput.c_str(),
            saddleTempHorse ? 1 : 0);
    }
}
