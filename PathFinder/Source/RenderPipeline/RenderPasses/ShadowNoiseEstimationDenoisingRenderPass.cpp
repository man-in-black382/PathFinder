#include "ShadowNoiseEstimationDenoisingRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    ShadowNoiseEstimationDenoisingRenderPass::ShadowNoiseEstimationDenoisingRenderPass()
        : RenderPass("ShadowNoiseEstimationDenoising") {}

    void ShadowNoiseEstimationDenoisingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::ShadowNoiseEstimationDenoising, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "ShadowNoiseEstimationDenoising.hlsl";
        });
    }

    void ShadowNoiseEstimationDenoisingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        /*  scheduler->ReadTexture(ResourceNames::ShadowNoiseEstimationOutput);

          ResourceScheduler::NewTextureProperties noiseEstimationProps{};
          noiseEstimationProps.ShaderVisibleFormat = HAL::ColorFormat::R8_Usigned_Norm;
          scheduler->NewTexture(ResourceNames::ShadowNoiseEstimationDenoisingOutput, noiseEstimationProps);*/
    }
     
    void ShadowNoiseEstimationDenoisingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        //context->GetCommandRecorder()->ApplyPipelineState(PSONames::ShadowNoiseEstimationDenoising);

        //auto resourceProvider = context->GetResourceProvider();

        //ShadowNoiseEstimationDenoisingCBContent cbContent{};
        //cbContent.NoiseEstimationTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::ShadowNoiseEstimationOutput);
        //cbContent.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::ShadowNoiseEstimationDenoisingOutput);

        //context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        //context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
