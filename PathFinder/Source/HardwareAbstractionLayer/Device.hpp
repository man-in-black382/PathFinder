#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "CommandAllocator.hpp"
#include "CommandList.hpp"
#include "CommandQueue.hpp"
#include "Fence.hpp"

namespace HAL
{
    class Device
    {
    public:
        Device();
        ~Device();

        CommandAllocator CreateCommandAllocator() const;
        CommandQueue CreateCommandQueue() const;
        Fence CreateFence() const;

        // Check for features, enumerate displays, display modes etc.
    private:
        Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
    };
}
