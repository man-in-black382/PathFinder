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
        /*   scheduler->ReadTexture(ResourceNames::ShadingStochasticShadowedOutput);
           scheduler->ReadTexture(ResourceNames::ShadingStochasticUnshadowedOutput);

           ResourceScheduler::NewTextureProperties noiseEstimationProps{};
           noiseEstimationProps.ShaderVisibleFormat = HAL::ColorFormat::R8_Usigned_Norm;
           scheduler->NewTexture(ResourceNames::ShadowNoiseEstimationOutput, noiseEstimationProps);*/
    }
     
    void ShadowNoiseEstimationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        //context->GetCommandRecorder()->ApplyPipelineState(PSONames::ShadowNoiseEstimation);

        //auto resourceProvider = context->GetResourceProvider();

        //ShadowNoiseEstimationCBContent cbContent{};
        //cbContent.StochasticShadowedLuminanceTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::ShadingStochasticShadowedOutput);
        //cbContent.StochasticUnshadowedLuminanceTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::ShadingStochasticUnshadowedOutput);
        //cbContent.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::ShadowNoiseEstimationOutput);
        //cbContent.MaximumLightsLuminance = context->GetContent()->GetSceneGPUStorage()->LightsMaximumLuminanance();

        //context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        //context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
