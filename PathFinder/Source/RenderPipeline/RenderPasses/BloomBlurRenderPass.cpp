#include "BloomBlurRenderPass.hpp"
#include "BlurCBContent.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    BloomBlurRenderPass::BloomBlurRenderPass()
        : RenderPass("BloomBlur") {}

    void BloomBlurRenderPass::ScheduleSubPasses(SubPassScheduler<RenderPassContentMediator>* scheduler)
    {
        if (!mMipGenerationSubPass)
        {
            mMipGenerationSubPass = std::make_unique<DownsamplingRenderSubPass>("BloomMipGeneration");
        }

        auto invocationInputs = GenerateDownsamplingShaderInvocationInputs(
            ResourceNames::CombinedShadingOversaturated,
            scheduler->GetTextureProperties(ResourceNames::CombinedShadingOversaturated),
            DownsamplingCBContent::Filter::Average,
            DownsamplingStrategy::WriteAllLevels);

        mMipGenerationSubPass->SetInvocationInputs(invocationInputs);
        scheduler->AddRenderSubPass(mMipGenerationSubPass.get());
    }

    void BloomBlurRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto fullMipRange = ResourceScheduler::MipSet::Range(0, std::nullopt);

        scheduler->ReadTexture(ResourceNames::CombinedShadingOversaturated, fullMipRange);
        scheduler->NewTexture(ResourceNames::BloomBlurIntermediate, fullMipRange, ResourceScheduler::NewTextureProperties{ ResourceNames::CombinedShadingOversaturated });
        scheduler->NewTexture(ResourceNames::BloomBlurOutput, fullMipRange, ResourceScheduler::NewTextureProperties{ ResourceNames::CombinedShadingOversaturated });
    }
     
    void BloomBlurRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        const BloomParameters& parameters = context->GetContent()->GetScene()->BloomParams();

        Blur(context, 0, parameters.SmallBlurRadius, parameters.SmallBlurSigma);
        Blur(context, 1, parameters.MediumBlurRadius, parameters.MediumBlurSigma);
        Blur(context, 2, parameters.LargeBlurRadius, parameters.LargeBlurSigma);
    }

    void BloomBlurRenderPass::Blur(RenderContext<RenderPassContentMediator>* context, uint8_t mipLevel, uint64_t radius, float sigma)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SeparableBlur);

        const auto& defaultRenderSurfaceDesc = context->GetDefaultRenderSurfaceDesc();
        
        auto resourceProvider = context->GetResourceProvider();
        auto dimensions = resourceProvider->GetTextureProperties(ResourceNames::CombinedShadingOversaturated).MipSize(mipLevel);

        SeparableBlurCBContent blurInputs{};

        // Blur horizontal
        auto kernel = Foundation::Gaussian::Kernel1D(radius, sigma);
        std::move(kernel.begin(), kernel.end(), blurInputs.Weights.begin());

        blurInputs.IsHorizontal = true;
        blurInputs.BlurRadius = radius;
        blurInputs.ImageSize = { dimensions.Width, dimensions.Height };
        blurInputs.InputTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::CombinedShadingOversaturated, mipLevel);
        blurInputs.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, mipLevel);
        blurInputs.MipLevel = mipLevel;

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(blurInputs);
        context->GetCommandRecorder()->Dispatch(dimensions, { 256, 1 });

        // Blur vertical
        std::swap(dimensions.Width, dimensions.Height);

        blurInputs.IsHorizontal = false;
        blurInputs.InputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurIntermediate, mipLevel);
        blurInputs.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomBlurOutput, mipLevel);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(blurInputs);
        context->GetCommandRecorder()->Dispatch(dimensions, { 256, 1 });
    }

}
