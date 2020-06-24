#include "DenoiserPreBlurRenderPass.hpp"
#include "BlurCBContent.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserPreBlurRenderPass::DenoiserPreBlurRenderPass()
        : RenderPass("DenoiserPreBlur") {}

    void DenoiserPreBlurRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
    }

    void DenoiserPreBlurRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        ResourceScheduler::NewTextureProperties outputProperties{};
        outputProperties.MipCount = 5;

        scheduler->NewTexture(ResourceNames::DenoisedPreBlurIntermediate);
        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingPreBlurred, outputProperties);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred, outputProperties);

        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingOutput);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingOutput);
    }
     
    void DenoiserPreBlurRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SeparableBlur);

        BlurTexture(context, ResourceNames::StochasticShadowedShadingOutput, ResourceNames::StochasticShadowedShadingPreBlurred);
        BlurTexture(context, ResourceNames::StochasticUnshadowedShadingOutput, ResourceNames::StochasticUnshadowedShadingPreBlurred);
    }

    void DenoiserPreBlurRenderPass::BlurTexture(RenderContext<RenderPassContentMediator>* context, Foundation::Name inputName, Foundation::Name outputName)
    {
        auto resourceProvider = context->GetResourceProvider();

        Geometry::Dimensions dispatchDimensions = resourceProvider->GetTextureProperties(inputName).Dimensions;

        BlurCBContent cbContent{};
        cbContent.BlurRadius = 5;
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
