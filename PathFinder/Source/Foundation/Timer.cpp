#include "Timer.hpp"
#include "StringUtils.hpp"

namespace Foundation
{

    ScopedTimer::ScopedTimer(const std::string& name)
        : mName{ name }, mStartTimestamp{ std::chrono::steady_clock::now() } {}

    ScopedTimer::~ScopedTimer()
    {
        auto newTimestamp = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(newTimestamp - mStartTimestamp);
        OutputDebugString((mName + ": " + std::to_string(elapsedTime.count()) + " us \n").c_str());
    }

}