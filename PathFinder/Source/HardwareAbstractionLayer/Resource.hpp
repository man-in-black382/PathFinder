#pragma once

#include <wrl.h>
#include <dxgi.h>
#include <cstdint>

#include "Device.hpp"

namespace HAL
{
    class Resource {
    public:
        Resource(const Device& device);
        Resource(const Microsoft::WRL::ComPtr<IDXGISwapChain>& swapChain, uint8_t bufferIndex);

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;

    public:
        inline const auto D3DPtr() const { return mResource.Get(); }
    };
}

