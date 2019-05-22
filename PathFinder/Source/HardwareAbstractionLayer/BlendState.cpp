#include "BlendState.hpp"

#include <type_traits>

namespace HAL
{

    BlendState::BlendState()
    {
        for (auto rt = 0; rt < 8; rt++)
        {
            mDesc.RenderTarget[rt].RenderTargetWriteMask = 0x0F;
        }
    }

    void BlendState::SetSourceValues(Value color, Value alpha, RenderTarget renderTarget)
    {
        D3D12_BLEND colorValue;
        D3D12_BLEND alphaValue;

        switch (color) {
        case Value::Original: colorValue = D3D12_BLEND_SRC_COLOR; break;
        case Value::Inverse: colorValue = D3D12_BLEND_INV_SRC_COLOR; break;
        case Value::Zero: colorValue = D3D12_BLEND_ZERO; break;
        case Value::One: colorValue = D3D12_BLEND_ONE; break;
        }

        switch (alpha) {
        case Value::Original: alphaValue = D3D12_BLEND_SRC_ALPHA; break;
        case Value::Inverse: alphaValue = D3D12_BLEND_INV_SRC_ALPHA; break;
        case Value::Zero: alphaValue = D3D12_BLEND_ZERO; break;
        case Value::One: alphaValue = D3D12_BLEND_ONE; break;
        }

        auto rtIdx = std::underlying_type<RenderTarget>::type(renderTarget);
        mDesc.RenderTarget[rtIdx].SrcBlend = colorValue;
        mDesc.RenderTarget[rtIdx].SrcBlendAlpha = alphaValue;
    }

    void BlendState::SetDestinationValues(Value color, Value alpha, RenderTarget renderTarget)
    {
        D3D12_BLEND colorValue;
        D3D12_BLEND alphaValue;

        switch (color) {
        case Value::Original: colorValue = D3D12_BLEND_DEST_COLOR; break;
        case Value::Inverse: colorValue = D3D12_BLEND_INV_DEST_COLOR; break;
        case Value::Zero: colorValue = D3D12_BLEND_ZERO; break;
        case Value::One: colorValue = D3D12_BLEND_ONE; break;
        }

        switch (alpha) {
        case Value::Original: alphaValue = D3D12_BLEND_DEST_ALPHA; break;
        case Value::Inverse: alphaValue = D3D12_BLEND_INV_DEST_ALPHA; break;
        case Value::Zero: alphaValue = D3D12_BLEND_ZERO; break;
        case Value::One: alphaValue = D3D12_BLEND_ONE; break;
        }

        auto rtIdx = std::underlying_type<RenderTarget>::type(renderTarget);
        mDesc.RenderTarget[rtIdx].DestBlend = colorValue;
        mDesc.RenderTarget[rtIdx].DestBlendAlpha = alphaValue;
    }

    void BlendState::SetFunctions(Function colorFunction, Function alphaFunction, RenderTarget renderTarget)
    {
        D3D12_BLEND_OP colorValue;
        D3D12_BLEND_OP alphaValue;

        switch (colorFunction)
        {
        case Function::Addition: colorValue = D3D12_BLEND_OP_ADD; break;
        case Function::Substraction: colorValue = D3D12_BLEND_OP_SUBTRACT; break;
        case Function::ReverseSubstraction: colorValue = D3D12_BLEND_OP_REV_SUBTRACT; break;
        case Function::Min: colorValue = D3D12_BLEND_OP_MIN; break;
        case Function::Max: colorValue = D3D12_BLEND_OP_MAX; break;
        }

        switch (alphaFunction)
        {
        case Function::Addition: alphaValue = D3D12_BLEND_OP_ADD; break;
        case Function::Substraction: alphaValue = D3D12_BLEND_OP_SUBTRACT; break;
        case Function::ReverseSubstraction: alphaValue = D3D12_BLEND_OP_REV_SUBTRACT; break;
        case Function::Min: alphaValue = D3D12_BLEND_OP_MIN; break;
        case Function::Max: alphaValue = D3D12_BLEND_OP_MAX; break;
        }

        auto rtIdx = std::underlying_type<RenderTarget>::type(renderTarget);
        mDesc.RenderTarget[rtIdx].BlendOp = colorValue;
        mDesc.RenderTarget[rtIdx].BlendOpAlpha = alphaValue;
    }

    void BlendState::SetBlendingEnabled(bool enabled, RenderTarget renderTarget)
    {
        auto rtIdx = std::underlying_type<RenderTarget>::type(renderTarget);
        mDesc.RenderTarget[rtIdx].BlendEnable = enabled;
    }

}
