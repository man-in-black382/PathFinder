#include "BloomBlurRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

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

        stateCreator->CreateComputeState(PSONames::BloomDownscaling, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "BloomDownscaling.hlsl";
        });
    }

    void BloomBlurRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        ResourceScheduler::NewTextureProperties blurTextureProps{};
        blurTextureProps.MipCount = 3; 

        //scheduler->ReadWriteTexture(ResourceNames::DeferredLightingOverexposedOutput);
        scheduler->NewTexture(ResourceNames::BloomBlurIntermediate, blurTextureProps);
        scheduler->NewTexture(ResourceNames::BloomBlurOutput, blurTextureProps);
    }
     
    void BloomBlurRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        /* BlurFullResolution(context);
         DownscaleAndBlurHalfResolution(context);
         DownscaleAndBlurQuadResolution(context);*/
    }

    void BloomBlurRenderPass::BlurFullResolution(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BloomBlur);

        const auto& defaultRenderSurfaceDesc = context->GetDefaultRenderSurfaceDesc();
        auto fullResDimensions = defaultRenderSurfaceDesc.Dimensions();
        auto resourceProvider = context->GetResourceProvider();

        BloomBlurCBContent blurInputs{};

        // Blur horizontal
        const BloomParameters& parameters = context->GetContent()->GetScene()->BloomParams();
        auto kernel = Foundation::Gaussian::Kernel1D(parameters.SmallBlurRadius, parameters.SmallBlurSigma);
        std::move(kernel.begin(), kernel.end(), blurInputs.Weights.begin());

        blurInputs.IsHorizontal = true;
        blurInputs.BlurRadius = parameters.SmallBlurRadius;
        blurInputs.ImageSize = { fullResDimensions.Width, fullResDimensions.Height };
        blurInputs.InputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingOverexposedOutput);
        blurInputs.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 0);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(blurInputs);
        context->GetCommandRecorder()->Dispatch(fullResDimensions, { 256, 1 });

        // Blur vertical
        std::swap(fullResDimensions.Width, fullResDimensions.Height);

        blurInputs.IsHorizontal = false;
        blurInputs.InputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 0);
        blurInputs.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 0);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(blurInputs);
        context->GetCommandRecorder()->Dispatch(fullResDimensions, { 256, 1 });
    }

    void BloomBlurRenderPass::DownscaleAndBlurHalfResolution(RenderContext<RenderPassContentMediator>* context)
    {
        const auto& defaultRenderSurfaceDesc = context->GetDefaultRenderSurfaceDesc();
        auto halfResDimensions = defaultRenderSurfaceDesc.Dimensions().XYMultiplied(0.5);
        auto resourceProvider = context->GetResourceProvider();

        // Downscale
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BloomDownscaling);

        BloomDownscalingCBContent downscalingInputs{};
        downscalingInputs.FullResSourceTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 0);
        downscalingInputs.HalfResDestinationTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 1);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(downscalingInputs);
        context->GetCommandRecorder()->Dispatch(halfResDimensions, { 8, 8 });

        // Blur
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BloomBlur);

        BloomBlurCBContent blurInputs{};

        // Blur horizontal
        const BloomParameters& parameters = context->GetContent()->GetScene()->BloomParams();
        auto kernel = Foundation::Gaussian::Kernel1D(parameters.MediumBlurRadius, parameters.MediumBlurSigma);
        std::move(kernel.begin(), kernel.end(), blurInputs.Weights.begin());

        blurInputs.IsHorizontal = true;
        blurInputs.BlurRadius = parameters.MediumBlurRadius;
        blurInputs.ImageSize = { halfResDimensions.Width, halfResDimensions.Height };
        blurInputs.InputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 1);
        blurInputs.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 1);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(blurInputs);
        context->GetCommandRecorder()->Dispatch(halfResDimensions, { 256, 1 });

        // Blur vertical
        std::swap(halfResDimensions.Width, halfResDimensions.Height);

        blurInputs.IsHorizontal = false;
        blurInputs.InputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 1);
        blurInputs.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 1);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(blurInputs);
        context->GetCommandRecorder()->Dispatch(halfResDimensions, { 256, 1 });
    }

    void BloomBlurRenderPass::DownscaleAndBlurQuadResolution(RenderContext<RenderPassContentMediator>* context)
    {
        const auto& defaultRenderSurfaceDesc = context->GetDefaultRenderSurfaceDesc();
        auto quadResDimensions = defaultRenderSurfaceDesc.Dimensions().XYMultiplied(0.25);
        auto resourceProvider = context->GetResourceProvider();

        // Downscale
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BloomDownscaling);

        BloomDownscalingCBContent downscalingInputs{};
        downscalingInputs.FullResSourceTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 1);
        downscalingInputs.HalfResDestinationTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 2);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(downscalingInputs);
        context->GetCommandRecorder()->Dispatch(quadResDimensions, { 8, 8 });

        // Blur
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BloomBlur);

        BloomBlurCBContent blurInputs{};

        // Blur horizontal
        const BloomParameters& parameters = context->GetContent()->GetScene()->BloomParams();
        auto kernel = Foundation::Gaussian::Kernel1D(parameters.LargeBlurRadius, parameters.LargeBlurSigma);
        std::move(kernel.begin(), kernel.end(), blurInputs.Weights.begin());

        blurInputs.IsHorizontal = true;
        blurInputs.BlurRadius = parameters.LargeBlurRadius;
        blurInputs.ImageSize = { quadResDimensions.Width, quadResDimensions.Height };
        blurInputs.InputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 2);
        blurInputs.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 2);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(blurInputs);
        context->GetCommandRecorder()->Dispatch(quadResDimensions, { 256, 1 });

        // Blur vertical
        std::swap(quadResDimensions.Width, quadResDimensions.Height);

        blurInputs.IsHorizontal = false;
        blurInputs.InputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, 2);
        blurInputs.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, 2);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(blurInputs);
        context->GetCommandRecorder()->Dispatch(quadResDimensions, { 256, 1 });
    }

}
