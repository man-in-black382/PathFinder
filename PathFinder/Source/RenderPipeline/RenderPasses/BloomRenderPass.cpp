#include "BloomRenderPass.hpp"

namespace PathFinder
{

    BloomRenderPass::BloomRenderPass()
        : RenderPass("Bloom") {}

    void BloomRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::BloomDownscaling, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "BloomDownscaling.hlsl";
        });
    }

    void BloomRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        ResourceScheduler::NewTextureProperties bloomTextureProps{};
        bloomTextureProps.MipCount = 2; // To store half and quad versions of overexposed deferred lighting output
        bloomTextureProps.Dimensions = scheduler->DefaultRenderSurfaceDesc().Dimensions().XYMultiplied(0.5);

        scheduler->ReadTexture(ResourceNames::DeferredLightingOverexposedOutput);
        scheduler->NewTexture(ResourceNames::DeferredLightingOverexposedOutputDownscaled, bloomTextureProps);
    }
     
    void BloomRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BloomDownscaling);

        auto resourceProvider = context->GetResourceProvider();
        const auto& defaultRenderSurfaceDesc = context->GetDefaultRenderSurfaceDesc();
        const auto& fullResDimensions = defaultRenderSurfaceDesc.Dimensions();

        BloomDownscalingCBContent downscalingInputs{};
        downscalingInputs.SourceTextureInverseDimensions = glm::vec2{ 1.0f / fullResDimensions.Width, 1.0f / fullResDimensions.Height };
        downscalingInputs.SourceTextureSRIndex = resourceProvider->GetSRTextureIndex(ResourceNames::DeferredLightingOverexposedOutput);
        downscalingInputs.HalfSizeDestinationTextureUAIndex = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingOverexposedOutputDownscaled, 0);
        downscalingInputs.QuadSizeDestinationTextureUAIndex = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingOverexposedOutputDownscaled, 1);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(downscalingInputs);
        context->GetCommandRecorder()->Dispatch(fullResDimensions.XYMultiplied(0.5), { 8, 8 });

        /*BlurCBContent cbContent{};
        cbContent.BlurRadius = 20;
        auto kernel = Foundation::GaussianFunction::Produce1DKernel(20);
        std::move(kernel.begin(), kernel.end(), cbContent.Weights.begin());

        cbContent.InputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferRT0);
        cbContent.OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::BloomFullResolution);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(5, 720, 1);*/
    }

}
