#include "Log.h"

#include <Windows.h>

#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace StabledDonkey
{
    namespace
    {
        constexpr const char *LOG_FILE = "StabledDonkey.log";
        std::mutex g_logMutex;
    }

    void StartLogSession()
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        FILE *file = nullptr;
        fopen_s(&file, LOG_FILE, "w");
        if (file)
            fclose(file);
    }

    void Log(const char *format, ...)
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        FILE *file = nullptr;
        fopen_s(&file, LOG_FILE, "a");
        if (!file)
            return;

        SYSTEMTIME time{};
        GetLocalTime(&time);
        fprintf(file, "[%02u:%02u:%02u] ", time.wHour, time.wMinute, time.wSecond);

        va_list args;
        va_start(args, format);
        vfprintf(file, format, args);
        va_end(args);
        fprintf(file, "\n");
        fclose(file);
    }
}
