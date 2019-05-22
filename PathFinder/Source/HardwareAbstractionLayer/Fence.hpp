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
    
    private:
        Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
        uint64_t mExpectedValue = 0;
    
    public:
        inline const auto D3DPtr() const { return mFence.Get(); }
        inline const auto ExpectedValue() const { return mExpectedValue; }
        inline const auto CompletedValue() const { return mFence->GetCompletedValue(); }
    };
}

