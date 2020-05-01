#include "DenoiserReprojectionRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserReprojectionRenderPass::DenoiserReprojectionRenderPass()
        : RenderPass("DenoiserReprojection") {}

    void DenoiserReprojectionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserReprojection, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "ShadowDenoising.hlsl";
        });
    }

    void DenoiserReprojectionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        ResourceScheduler::NewTextureProperties frameCountProperties{};
        frameCountProperties.ShaderVisibleFormat = HAL::ColorFormat::R8_Unsigned;
        frameCountProperties.TextureCount = 2;

        scheduler->NewTexture(ResourceNames::DenoiserReprojectedFramesCount, frameCountProperties);
        scheduler->ReadTexture(ResourceNames::GBufferRT0);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
    }
     
    void DenoiserReprojectionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserReprojection);

        //auto resourceProvider = context->GetResourceProvider();
        //auto dimensions = context->GetDefaultRenderSurfaceDesc().Dimensions();

        //ShadowDenoisingCBContent cbContent{};
        //cbContent.AnalyticLuminanceTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::ShadingAnalyticOutput);
        //cbContent.StochasticShadowedLuminanceTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticShadowedOutput);
        //cbContent.StochasticUnshadowedLuminanceTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticUnshadowedOutput);
        //cbContent.NoiseEstimationTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::ShadowNoiseEstimationDenoisingOutput);
        //cbContent.GBufferTextureIndex = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GBufferRT0);
        //cbContent.DepthTextureIndex = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        //cbContent.IntermediateOutput0TextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DenoisingStochasticShadowedIntermediateTarget);
        //cbContent.IntermediateOutput1TextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DenoisingStochasticUnsadowedIntermediateTarget);
        //cbContent.MaximumLightsLuminance = context->GetContent()->GetSceneGPUStorage()->LightsMaximumLuminanance();
        //cbContent.ImageSize = { dimensions.Width, dimensions.Height };

        //// Denoise horizontally
        //cbContent.IsHorizontal = true;

        //context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        //context->GetCommandRecorder()->Dispatch(dimensions, { 256, 1 });

        //// Denoise vertically
        //std::swap(dimensions.Width, dimensions.Height);
        //cbContent.IsHorizontal = false;
        //cbContent.StochasticShadowedLuminanceTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DenoisingStochasticShadowedIntermediateTarget);
        //cbContent.StochasticUnshadowedLuminanceTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DenoisingStochasticUnsadowedIntermediateTarget);
        //cbContent.FinalOutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::ShadowDenoisingOutput);

        //context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        //context->GetCommandRecorder()->Dispatch(dimensions, { 256, 1 });
    }

}
