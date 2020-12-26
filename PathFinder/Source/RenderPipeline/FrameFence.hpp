#pragma once

#include <HardwareAbstractionLayer/Fence.hpp>

namespace PathFinder
{
    class FrameFence
    {
    public:
        FrameFence(const HAL::Device& device);

        void StallCurrentThreadUntilCompletion(uint8_t allowedSimultaneousFramesCount = 1);
    
    private:
        HAL::Fence mFence;
    
    public:
        inline const HAL::Fence& HALFence() const { return mFence; }
        inline HAL::Fence& HALFence() { return mFence; }
    };
}

