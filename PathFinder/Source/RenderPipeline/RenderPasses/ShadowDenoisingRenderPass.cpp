#include "ShadowDenoisingRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    ShadowDenoisingRenderPass::ShadowDenoisingRenderPass()
        : RenderPass("ShadowDenoising") {}

    void ShadowDenoisingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::ShadowDenoising, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "ShadowDenoising.hlsl";
        });
    }

    void ShadowDenoisingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        //scheduler->ReadWriteTexture(ResourceNames::ShadingStochasticShadowedOutput);
        //scheduler->ReadWriteTexture(ResourceNames::ShadingStochasticUnshadowedOutput);

        //scheduler->ReadTexture(ResourceNames::ShadingAnalyticOutput);
        //scheduler->ReadTexture(ResourceNames::ShadowNoiseEstimationDenoisingOutput);
        //scheduler->ReadTexture(ResourceNames::GBufferRT0);
        //scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);

        //scheduler->NewTexture(ResourceNames::DenoisingStochasticShadowedIntermediateTarget);
        //scheduler->NewTexture(ResourceNames::DenoisingStochasticUnsadowedIntermediateTarget);
        //scheduler->NewTexture(ResourceNames::ShadowDenoisingOutput);
    }
     
    void ShadowDenoisingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        //context->GetCommandRecorder()->ApplyPipelineState(PSONames::ShadowDenoising);

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
