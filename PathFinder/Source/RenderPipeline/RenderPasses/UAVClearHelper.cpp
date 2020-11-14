#include "UAVClearHelper.hpp"
#include "PipelineNames.hpp"

#include <algorithm>
#include <cmath>

namespace PathFinder
{

    void ClearUAV(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, UAVClearCBContent& cbContent)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::UAVClear);

        Geometry::Dimensions dispatchDimensions{};

        switch (cbContent.Operation)
        {
        case UAVClearCBContent::ClearOp::TextureFloat:
        case UAVClearCBContent::ClearOp::TextureUInt:
            cbContent.OutputTexIdx = context->GetResourceProvider()->GetUATextureIndex(resourceName);
            dispatchDimensions = context->GetResourceProvider()->GetTextureProperties(resourceName).Dimensions;
            break;

        case UAVClearCBContent::ClearOp::BufferFloat:
            context->GetCommandRecorder()->BindBuffer(resourceName, 0, 0, HAL::ShaderRegister::UnorderedAccess); 
            dispatchDimensions.Width = context->GetResourceProvider()->GetBufferProperties(resourceName).Size / sizeof(float);
            break;
        case UAVClearCBContent::ClearOp::BufferUInt:
            context->GetCommandRecorder()->BindBuffer(resourceName, 1, 0, HAL::ShaderRegister::UnorderedAccess);
            dispatchDimensions.Width = context->GetResourceProvider()->GetBufferProperties(resourceName).Size / sizeof(uint32_t);
            break;
        }
        
        cbContent.BufferSize = dispatchDimensions.Width;

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(dispatchDimensions, { 256, 1 });
    }

    void ClearUAVTextureFloat(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, const glm::vec4& clearValues)
    {
        UAVClearCBContent cbContent{};
        cbContent.Operation = UAVClearCBContent::ClearOp::TextureFloat;
        cbContent.FloatValues = clearValues;
        ClearUAV(context, resourceName, cbContent);
    }

    void ClearUAVTextureUInt(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, const glm::uvec4& clearValues)
    {
        UAVClearCBContent cbContent{};
        cbContent.Operation = UAVClearCBContent::ClearOp::TextureUInt;
        cbContent.UIntValues = clearValues;
        ClearUAV(context, resourceName, cbContent);
    }

    void ClearUAVBufferFloat(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, float clearValue)
    {
        UAVClearCBContent cbContent{};
        cbContent.Operation = UAVClearCBContent::ClearOp::BufferFloat;
        cbContent.FloatValues = { clearValue, clearValue, clearValue, clearValue };
        ClearUAV(context, resourceName, cbContent);
    }

    void ClearUAVBufferUInt(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, uint32_t clearValue)
    {
        UAVClearCBContent cbContent{};
        cbContent.Operation = UAVClearCBContent::ClearOp::BufferUInt;
        cbContent.UIntValues = { clearValue, clearValue, clearValue, clearValue };
        ClearUAV(context, resourceName, cbContent);
    }

}
