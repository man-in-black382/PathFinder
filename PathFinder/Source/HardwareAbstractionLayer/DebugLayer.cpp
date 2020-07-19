#include "DebugLayer.hpp"
#include "Utils.h"

#include <d3d12.h>
#include <wrl.h>

namespace HAL
{

    void EnableDebugLayer()
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }

}

