#include "SwapChain.hpp"
#include "Utils.h"

namespace HAL
{

    Resource::Resource(const Device& device)
    {
        
    }

    Resource::Resource(const Microsoft::WRL::ComPtr<IDXGISwapChain>& swapChain, uint8_t bufferIndex)
    {
        ThrowIfFailed(swapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&mResource)));
    }

}
