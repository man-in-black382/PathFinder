#include "UAVClearHelper.hpp"
#include "PipelineNames.hpp"

#include <algorithm>
#include <cmath>

namespace PathFinder
{

    void ClearUAV(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, UAVClearCBContent& cbContent)
    {
        cbContent.OutputTexIdx = context->GetResourceProvider()->GetUATextureIndex(resourceName);
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::UAVClear);
        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetResourceProvider()->GetTextureProperties(resourceName).Dimensions, { 16, 16 });
    }

    void ClearUAVFloat(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, const glm::vec4& clearValues)
    {
        UAVClearCBContent cbContent{};
        cbContent.Type = UAVClearCBContent::TextureType::Float;
        cbContent.FloatValues = clearValues;
        ClearUAV(context, resourceName, cbContent);
    }

    void ClearUAVUInt(RenderContext<RenderPassContentMediator>* context, Foundation::Name resourceName, const glm::uvec4& clearValues)
    {
        UAVClearCBContent cbContent{};
        cbContent.Type = UAVClearCBContent::TextureType::UInt;
        cbContent.UIntValues = clearValues;
        ClearUAV(context, resourceName, cbContent);
    }

}
