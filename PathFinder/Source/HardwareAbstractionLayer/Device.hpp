#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "DisplayAdapter.hpp"

namespace HAL
{
    class Device
    {
    public:
        Device(const DisplayAdapter& adapter);

    private:
        Microsoft::WRL::ComPtr<ID3D12Device5> mDevice;

    public:
        inline ID3D12Device5* D3DDevice() const { return mDevice.Get(); }
    };
}
