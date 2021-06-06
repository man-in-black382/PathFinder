#include "DenoiserPreBlurRenderPass.hpp"
#include "BlurCBContent.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    DenoiserPreBlurRenderPass::DenoiserPreBlurRenderPass()
        : RenderPass("DenoiserPreBlur") {}

    void DenoiserPreBlurRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
    }

    void DenoiserPreBlurRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        // Non depth aware pre-blur leaks light when blurring in main pass.
        // Disable and come up with more robust solution.
        return;

        if (!scheduler->GetContent()->GetSettings()->IsDenoiserEnabled)
            return;

        auto currentFrameIndex = scheduler->GetFrameNumber() % 2;

        NewTextureProperties outputProperties{};
        outputProperties.MipCount = 5;

        scheduler->NewTexture(ResourceNames::DenoisedPreBlurIntermediate);
        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingPreBlurred, MipSet::FirstMip(), outputProperties);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred, MipSet::FirstMip(), outputProperties);

        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingOutput[currentFrameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingOutput);
    }
     
    void DenoiserPreBlurRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SeparableBlur);

        auto currentFrameIndex = context->GetFrameNumber() % 2;

        BlurTexture(context, ResourceNames::StochasticShadowedShadingOutput[currentFrameIndex], ResourceNames:: StochasticShadowedShadingPreBlurred);
        BlurTexture(context, ResourceNames::StochasticUnshadowedShadingOutput, ResourceNames::StochasticUnshadowedShadingPreBlurred);
    }

    void DenoiserPreBlurRenderPass::BlurTexture(RenderContext<RenderPassContentMediator>* context, Foundation::Name inputName, Foundation::Name outputName)
    {
        auto resourceProvider = context->GetResourceProvider();

        Geometry::Dimensions dispatchDimensions = resourceProvider->GetTextureProperties(inputName).Dimensions;

        SeparableBlurCBContent cbContent{};
        cbContent.BlurRadius = context->GetContent()->GetSettings()->IsDenoiserEnabled ? 2 : 0;
        cbContent.IsHorizontal = true;
        cbContent.Weights.fill(1.0f / cbContent.BlurRadius);
        auto kernel = Foundation::Gaussian::Kernel1D(cbContent.BlurRadius);
        std::move(kernel.begin(), kernel.end(), cbContent.Weights.begin());
        cbContent.ImageSize = { dispatchDimensions.Width, dispatchDimensions.Height };

        cbContent.InputTexIdx = resourceProvider->GetSRTextureIndex(inputName);
        cbContent.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoisedPreBlurIntermediate);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(dispatchDimensions, { 256, 1 });

        // Blur vertical
        std::swap(dispatchDimensions.Width, dispatchDimensions.Height);
        cbContent.InputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoisedPreBlurIntermediate);
        cbContent.OutputTexIdx = resourceProvider->GetUATextureIndex(outputName);
        cbContent.IsHorizontal = false;

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(dispatchDimensions, { 256, 1 });
    }

}
