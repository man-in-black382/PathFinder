#include "Device.hpp"
#include "Utils.h"

HAL::Device::Device()
{
#if defined(DEBUG) || defined(_DEBUG) 
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    IUnknown *displayAdapter = nullptr; // Enumerate adapters properly later
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    ThrowIfFailed(D3D12CreateDevice(displayAdapter, featureLevel, IID_PPV_ARGS(&mDevice)));
}

HAL::Device::~Device()
{

}

HAL::CommandAllocator HAL::Device::CreateCommandAllocator() const
{

}

HAL::CommandQueue HAL::Device::CreateCommandQueue() const
{

}

Fence HAL::Device::CreateFence() const
{

}
