#include "BloomBlurAndCompositionRenderPass.hpp"

#include "../Foundation/GaussianFunction.hpp"

namespace PathFinder
{

    BloomBlurAndCompositionRenderPass::BloomBlurAndCompositionRenderPass()
        : RenderPass("BloomBlurAndComposition")
    {
        auto kernel = Foundation::GaussianFunction::Produce1DKernel(BloomBlurCBContent::MaximumRadius);
        std::move(kernel.begin(), kernel.end(), mCommonBlurCBContent.Weights.begin());
    }

    void BloomBlurAndCompositionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::BloomBlur, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "GaussianBlur.hlsl";
        });
    }

    void BloomBlurAndCompositionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        ResourceScheduler::NewTextureProperties blurTextureProps{};
        blurTextureProps.MipCount = 3; 

        scheduler->ReadWriteTexture(ResourceNames::DeferredLightingOverexposedOutput);
        scheduler->ReadWriteTexture(ResourceNames::DeferredLightingOverexposedOutputDownscaled);
        scheduler->NewTexture(ResourceNames::BloomBlurIntermediate, blurTextureProps);
        scheduler->NewTexture(ResourceNames::BloomBlurOutput, blurTextureProps);
    }
     
    void BloomBlurAndCompositionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BloomBlur);

        auto resourceProvider = context->GetResourceProvider();
        const auto& defaultRenderSurfaceDesc = context->GetDefaultRenderSurfaceDesc();
        const auto& fullResDimensions = defaultRenderSurfaceDesc.Dimensions();
        const auto& halfResDimensions = fullResDimensions.XYMultiplied(0.5);
        const auto& quadResDimensions = fullResDimensions.XYMultiplied(0.25);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(mCommonBlurCBContent);

        BloomBlurRootConstants constants{};

        // Blur horizontally
        constants.IsHorizontal = true;

        constants.BlurRadius = 20;
        constants.ImageSize = { fullResDimensions.Width, fullResDimensions.Height };
        constants.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingOverexposedOutput);
        constants.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 0);
        context->GetCommandRecorder()->SetRootConstants(constants, 1, 0);
        context->GetCommandRecorder()->Dispatch(fullResDimensions, { 256, 1 });

        constants.BlurRadius = 30;
        constants.ImageSize = { halfResDimensions.Width, halfResDimensions.Height };
        constants.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingOverexposedOutputDownscaled, 0);
        constants.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 1);
        context->GetCommandRecorder()->SetRootConstants(constants, 1, 0);
        context->GetCommandRecorder()->Dispatch(fullResDimensions, { 256, 1 });

        constants.BlurRadius = 40;
        constants.ImageSize = { quadResDimensions.Width, quadResDimensions.Height };
        constants.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingOverexposedOutputDownscaled, 1);
        constants.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 2);
        context->GetCommandRecorder()->SetRootConstants(constants, 1, 0);
        context->GetCommandRecorder()->Dispatch(fullResDimensions, { 256, 1 });

        // Blur vertically
        constants.IsHorizontal = false;

        constants.BlurRadius = 20;
        constants.ImageSize = { fullResDimensions.Width, fullResDimensions.Height };
        constants.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 0);
        constants.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 0);
        context->GetCommandRecorder()->SetRootConstants(constants, 1, 0);
        context->GetCommandRecorder()->Dispatch(fullResDimensions, { 256, 1 });

        constants.BlurRadius = 30;
        constants.ImageSize = { halfResDimensions.Width, halfResDimensions.Height };
        constants.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 1);
        constants.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 1);
        context->GetCommandRecorder()->SetRootConstants(constants, 1, 0);
        context->GetCommandRecorder()->Dispatch(fullResDimensions, { 256, 1 });

        constants.BlurRadius = 40;
        constants.ImageSize = { quadResDimensions.Width, quadResDimensions.Height };
        constants.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 2);
        constants.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 2);
        context->GetCommandRecorder()->SetRootConstants(constants, 1, 0);
        context->GetCommandRecorder()->Dispatch(fullResDimensions, { 256, 1 });
    }

}
