#include "Cooldown.hpp"

namespace Foundation
{

    Cooldown::Cooldown(float cooldownSeconds)
        : mCooldownTimeUS{ uint32_t(cooldownSeconds * 1000 * 1000) } 
    {
        mLastTimestamp = std::chrono::steady_clock::now();
    }

    bool Cooldown::Check()
    {
        auto newTimestamp = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(newTimestamp - mLastTimestamp);

        if (elapsedTime.count() > mCooldownTimeUS)
        {
            mLastTimestamp = newTimestamp;
            return true;
        }

        return false;
    }

}