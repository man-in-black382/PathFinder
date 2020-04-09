#include "ShadowNoiseEstimationRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    ShadowNoiseEstimationRenderPass::ShadowNoiseEstimationRenderPass()
        : RenderPass("ShadowNoiseEstimation") {}

    void ShadowNoiseEstimationRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::ShadowNoiseEstimation, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "ShadowNoiseEstimation.hlsl";
        });
    }

    void ShadowNoiseEstimationRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->ReadTexture(ResourceNames::ShadingStochasticShadowedOutput);
        scheduler->ReadTexture(ResourceNames::ShadingStochasticUnshadowedOutput);
        scheduler->NewTexture(ResourceNames::ShadowNoiseEstimationOutput);
    }
     
    void ShadowNoiseEstimationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        //context->GetCommandRecorder()->ApplyPipelineState(PSONames::ToneMapping);

        //ToneMappingCBContent cbContent{};
        //cbContent.InputTextureIndex = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::ShadingAnalyticalOutput/*BloomCompositionOutput*/);
        //cbContent.OutputTextureIndex = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::ToneMappingOutput);
        //cbContent.TonemappingParams = context->GetContent()->GetScene()->TonemappingParams();

        //context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        //auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(32, 32);
        //context->GetCommandRecorder()->Dispatch(dimensions.x, dimensions.y);
    }

}
