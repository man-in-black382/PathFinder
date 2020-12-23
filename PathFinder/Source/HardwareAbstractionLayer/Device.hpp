#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "GraphicAPIObject.hpp"

namespace HAL
{
    class DisplayAdapter;

    class Device : public GraphicAPIObject
    {
    public:
        Device(const DisplayAdapter& adapter, bool aftermathEnabled);

    private:
        Microsoft::WRL::ComPtr<ID3D12Device5> mDevice;

        bool mSupportsUniversalHeaps = false;
        bool mAftermathEnabled = false;
        uint64_t mMinimumHeapSize = 1;
        uint64_t mHeapAlignment = 1;
        uint64_t mNodeMask = 0;

    public:
        inline ID3D12Device5* D3DDevice() const { return mDevice.Get(); }

        inline auto SupportsUniversalHeaps() const { return mSupportsUniversalHeaps; }
        inline auto MinimumHeapSize() const { return mMinimumHeapSize; }
        inline auto MandatoryHeapAlignment() const { return mHeapAlignment; }
        inline auto AftermathEnabled() const { return mAftermathEnabled; }
        inline auto NodeMask() const { return mNodeMask; }
    };
}
