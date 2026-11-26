#pragma once

namespace StabledDonkey
{
    // Appends one line to StabledDonkey.log (game working directory).
    void Log(const char *format, ...);

    // Truncates the log; called once per game launch.
    void StartLogSession();
}
