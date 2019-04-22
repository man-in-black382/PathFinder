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
        ~Device();

    private:
        Microsoft::WRL::ComPtr<ID3D12Device> mDevice;

    public:
        inline const auto D3DPtr() const { return mDevice.Get(); }
    };
}
