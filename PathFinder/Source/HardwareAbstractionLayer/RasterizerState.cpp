#include "RasterizerState.hpp"

namespace HAL
{

    RasterizerState::RasterizerState()
    {
        SetCullMode(CullMode::Back);
        SetFillMode(FillMode::Solid);
        SetFrontClockwise(false);

        mDesc.DepthClipEnable = true;
    }

    void RasterizerState::SetFillMode(FillMode mode)
    {
        switch (mode) 
        {
        case FillMode::Wireframe: mDesc.FillMode = D3D12_FILL_MODE_WIREFRAME; break;
        case FillMode::Solid: mDesc.FillMode = D3D12_FILL_MODE_SOLID; break;
        }
    }

    void RasterizerState::SetCullMode(CullMode mode)
    {
        switch (mode)
        {
        case CullMode::None: mDesc.CullMode = D3D12_CULL_MODE_NONE; break;
        case CullMode::Front: mDesc.CullMode = D3D12_CULL_MODE_FRONT; break;
        case CullMode::Back: mDesc.CullMode = D3D12_CULL_MODE_BACK; break;
        }
    }

    void RasterizerState::SetFrontClockwise(bool frontIsClockwise)
    {
        mDesc.FrontCounterClockwise = !frontIsClockwise;
    }

}
