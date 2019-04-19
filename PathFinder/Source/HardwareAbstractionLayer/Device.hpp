#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "CommandAllocator.hpp"
#include "CommandList.hpp"
#include "CommandQueue.hpp"
#include "Fence.hpp"
#include "DisplayAdapter.hpp"
#include "DescriptorHeap.hpp"
#include "Descriptor.hpp"
#include "Resource.hpp"

namespace HAL
{
    class Device
    {
    public:
        Device(const DisplayAdapter& adapter);
        ~Device();

    private:
        Microsoft::WRL::ComPtr<ID3D12Device> mDevice;

    public:
        inline const auto Device() const { return mDevice.Get(); }
    };
}
