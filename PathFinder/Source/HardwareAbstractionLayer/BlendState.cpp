#include "BlendState.hpp"

#include <type_traits>

namespace HAL
{

    BlendState::BlendState()
    {
        for (auto rt = 0; rt < 8; rt++)
        {
            mDesc.RenderTarget[rt].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            mDesc.RenderTarget[rt].LogicOp = D3D12_LOGIC_OP_NOOP;
            mDesc.RenderTarget[rt].LogicOpEnable = false;
            mDesc.IndependentBlendEnable = true;

            SetSourceValues(Value::One, Value::One, RenderTarget(rt));
            SetDestinationValues(Value::Zero, Value::Zero, RenderTarget(rt));
            SetFunctions(Function::Addition, Function::Addition, RenderTarget(rt));
            SetBlendingEnabled(false, RenderTarget(rt));
        }
    }

    void BlendState::SetSourceValues(Value color, Value alpha, RenderTarget renderTarget)
    {
        auto rtIdx = std::underlying_type<RenderTarget>::type(renderTarget);
        mDesc.RenderTarget[rtIdx].SrcBlend = D3DValue(color);
        mDesc.RenderTarget[rtIdx].SrcBlendAlpha = D3DValue(alpha);
    }

    void BlendState::SetDestinationValues(Value color, Value alpha, RenderTarget renderTarget)
    {
        auto rtIdx = std::underlying_type<RenderTarget>::type(renderTarget);
        mDesc.RenderTarget[rtIdx].DestBlend = D3DValue(color);
        mDesc.RenderTarget[rtIdx].DestBlendAlpha = D3DValue(alpha);
    }

    void BlendState::SetFunctions(Function colorFunction, Function alphaFunction, RenderTarget renderTarget)
    {
        D3D12_BLEND_OP colorValue{};
        D3D12_BLEND_OP alphaValue{};

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

    D3D12_BLEND BlendState::D3DValue(Value value)
    {
        switch (value)
        {
        case Value::Zero: return D3D12_BLEND_ZERO;
        case Value::One: return D3D12_BLEND_ONE;
        case Value::SourceColor: return D3D12_BLEND_SRC_COLOR;
        case Value::InverveSourceColor: return D3D12_BLEND_INV_SRC_COLOR;
        case Value::SourceAlpha: return D3D12_BLEND_SRC_ALPHA;
        case Value::InverseSourceAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
        case Value::DestinationAlpha: return D3D12_BLEND_DEST_ALPHA;
        case Value::InverseDestinationAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
        case Value::DestinationColor: return D3D12_BLEND_DEST_COLOR;
        case Value::InverseDestinationColor: return D3D12_BLEND_INV_DEST_COLOR;
        default: assert_format(false, "Should never be hit"); return D3D12_BLEND_ONE;
        }
    }

}
