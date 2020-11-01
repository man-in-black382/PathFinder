#include "Device.hpp"
#include "Utils.h"
#include "DisplayAdapter.hpp"

#include <aftermath/GFSDK_Aftermath.h>

namespace HAL
{
    Device::Device(const DisplayAdapter& adapter, bool aftermathEnabled)
        : mAftermathEnabled{ aftermathEnabled }
    {
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_1;
        ThrowIfFailed(D3D12CreateDevice(adapter.D3DAdapter(), featureLevel, IID_PPV_ARGS(&mDevice)));

        mHeapAlignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        mMinimumHeapSize = mHeapAlignment;

        D3D12_FEATURE_DATA_D3D12_OPTIONS featureSupport{};
        ThrowIfFailed(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureSupport, sizeof(featureSupport)));

        switch (featureSupport.ResourceHeapTier)
        {
        case D3D12_RESOURCE_HEAP_TIER_1: mSupportsUniversalHeaps = false; break;
        default: mSupportsUniversalHeaps = true; break;
        }
    }
}
