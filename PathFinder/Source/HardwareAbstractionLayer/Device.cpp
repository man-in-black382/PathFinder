#include "Device.hpp"
#include "Utils.h"

namespace HAL
{
    Device::Device(const DisplayAdapter& adapter)
    {
    #if defined(DEBUG) || defined(_DEBUG) 
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    #endif

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_1;
        ThrowIfFailed(D3D12CreateDevice(adapter.D3DPtr(), featureLevel, IID_PPV_ARGS(&mDevice)));
    }

}
