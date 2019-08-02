#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>

#include "Device.hpp"

namespace HAL
{
    class Fence
    {
    public:
        Fence(const Device& device);

        void IncreaseExpectedValue();
        bool IsCompleted() const;
        void SetCompletionEventHandle(HANDLE handle);
        void StallCurrentThreadUntilCompletion(uint8_t allowedSimultaneousFramesCount = 1);
    
    private:
        Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
        uint64_t mExpectedValue = 0;
    
    public:
        inline ID3D12Fence* D3DFence() const { return mFence.Get(); }
        inline uint64_t ExpectedValue() const { return mExpectedValue; }
        inline uint64_t CompletedValue() const { return mFence->GetCompletedValue(); }
    };
}

