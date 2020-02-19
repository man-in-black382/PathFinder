#include "ToneMappingRenderPass.hpp"

#include "../Foundation/GaussianFunction.hpp"

namespace PathFinder
{

    ToneMappingRenderPass::ToneMappingRenderPass()
        : RenderPass("ToneMapping") {}

    void ToneMappingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::ToneMapping, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"ToneMappingRenderPass.hlsl";
        });
    }

    void ToneMappingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->ReadTexture(ResourceNames::DeferredLightingOutput);
        scheduler->NewTexture(ResourceNames::ToneMappingOutput);
    }
     
    void ToneMappingRenderPass::Render(RenderContext* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::ToneMapping);

        ToneMappingCBContent cbContent{};
        cbContent.InputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::DeferredLightingOutput);
        cbContent.OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::ToneMappingOutput);
        cbContent.TonemappingParams = context->GetScene()->TonemappingParams();

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(32, 32);
        context->GetCommandRecorder()->Dispatch(dimensions.x, dimensions.y);
    }

}
