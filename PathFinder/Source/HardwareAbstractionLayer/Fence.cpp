#include "Fence.hpp"
#include "Utils.h"

namespace HAL
{
  
    Fence::Fence(const Microsoft::WRL::ComPtr<ID3D12Device>& device)
    {
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    }

}

