#include "BloomBlurRenderPass.hpp"

#include "../Foundation/GaussianFunction.hpp"

namespace PathFinder
{

    BloomBlurRenderPass::BloomBlurRenderPass()
        : RenderPass("BloomBlur") {}

    void BloomBlurRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::BloomBlur, [this](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "BloomBlur.hlsl";
        });
    }

    void BloomBlurRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        ResourceScheduler::NewTextureProperties blurTextureProps{};
        blurTextureProps.MipCount = 3; 

        scheduler->ReadWriteTexture(ResourceNames::DeferredLightingOverexposedOutput);
        scheduler->ReadWriteTexture(ResourceNames::DeferredLightingOverexposedOutputDownscaled);
        scheduler->NewTexture(ResourceNames::BloomBlurIntermediate, blurTextureProps);
        scheduler->NewTexture(ResourceNames::BloomBlurOutput, blurTextureProps);
    }
     
    void BloomBlurRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BloomBlur);

        BlurFullResolution(context);
        BlurHalfResolution(context);
        BlurQuadResolution(context);
    }

    void BloomBlurRenderPass::BlurFullResolution(RenderContext<RenderPassContentMediator>* context)
    {
        const auto& defaultRenderSurfaceDesc = context->GetDefaultRenderSurfaceDesc();
        auto fullResDimensions = defaultRenderSurfaceDesc.Dimensions();
        auto resourceProvider = context->GetResourceProvider();

        BloomBlurCBContent cbContent{};

        // Blur horizontal
        auto kernel = Foundation::GaussianFunction::Produce1DKernel(mFullResBlurRadius);
        std::move(kernel.begin(), kernel.end(), cbContent.Weights.begin());

        cbContent.IsHorizontal = true;
        cbContent.BlurRadius = mFullResBlurRadius;
        cbContent.ImageSize = { fullResDimensions.Width, fullResDimensions.Height };
        cbContent.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingOverexposedOutput);
        cbContent.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 0);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(fullResDimensions, { 256, 1 });

        // Blur vertical
        std::swap(fullResDimensions.Width, fullResDimensions.Height);

        cbContent.IsHorizontal = false;
        cbContent.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 0);
        cbContent.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 0);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(fullResDimensions, { 256, 1 });
    }

    void BloomBlurRenderPass::BlurHalfResolution(RenderContext<RenderPassContentMediator>* context)
    {
        const auto& defaultRenderSurfaceDesc = context->GetDefaultRenderSurfaceDesc();
        auto halfResDimensions = defaultRenderSurfaceDesc.Dimensions().XYMultiplied(0.5);
        auto resourceProvider = context->GetResourceProvider();

        BloomBlurCBContent cbContent{};

        // Blur horizontal
        auto kernel = Foundation::GaussianFunction::Produce1DKernel(mHalfResBlurRadius);
        std::move(kernel.begin(), kernel.end(), cbContent.Weights.begin());

        cbContent.IsHorizontal = true;
        cbContent.BlurRadius = mHalfResBlurRadius;
        cbContent.ImageSize = { halfResDimensions.Width, halfResDimensions.Height };
        cbContent.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingOverexposedOutputDownscaled, 0);
        cbContent.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 1);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(halfResDimensions, { 256, 1 });

        // Blur vertical
        std::swap(halfResDimensions.Width, halfResDimensions.Height);

        cbContent.IsHorizontal = false;
        cbContent.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 1);
        cbContent.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 1);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(halfResDimensions, { 256, 1 });
    }

    void BloomBlurRenderPass::BlurQuadResolution(RenderContext<RenderPassContentMediator>* context)
    {
        const auto& defaultRenderSurfaceDesc = context->GetDefaultRenderSurfaceDesc();
        auto quadResDimensions = defaultRenderSurfaceDesc.Dimensions().XYMultiplied(0.25);
        auto resourceProvider = context->GetResourceProvider();

        BloomBlurCBContent cbContent{};

        // Blur horizontal
        auto kernel = Foundation::GaussianFunction::Produce1DKernel(mQuadResBlurRadius);
        std::move(kernel.begin(), kernel.end(), cbContent.Weights.begin());

        cbContent.IsHorizontal = true;
        cbContent.BlurRadius = mHalfResBlurRadius;
        cbContent.ImageSize = { quadResDimensions.Width, quadResDimensions.Height };
        cbContent.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingOverexposedOutputDownscaled, 1);
        cbContent.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 2);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(quadResDimensions, { 256, 1 });

        // Blur vertical
        std::swap(quadResDimensions.Width, quadResDimensions.Height);

        cbContent.IsHorizontal = false;
        cbContent.InputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 2);
        cbContent.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 2);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(quadResDimensions, { 256, 1 });
    }

}
