#pragma once

#include <chrono>

namespace Foundation
{

    class ScopedTimer
    {
    public:
        ScopedTimer(const std::string& name);
        ~ScopedTimer();

    private:
        std::string mName;
        std::chrono::time_point<std::chrono::steady_clock> mStartTimestamp;
    };

}
