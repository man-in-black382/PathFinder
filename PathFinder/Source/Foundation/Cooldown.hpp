#pragma once

#include <chrono>

namespace Foundation
{

    class Cooldown
    {
    public:
        Cooldown(float cooldownSeconds);
        
        bool CheckThenUpdate();

    private:
        uint32_t mCooldownTimeUS;
        std::chrono::time_point<std::chrono::steady_clock> mLastTimestamp;
    };

}
