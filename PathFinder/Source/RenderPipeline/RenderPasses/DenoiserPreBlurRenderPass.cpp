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
        scheduler->NewTexture(ResourceNames::DenoisedPreBlurIntermediate);
        scheduler->ReadWriteTexture(ResourceNames::StochasticShadowedShadingOutput);
        scheduler->ReadWriteTexture(ResourceNames::StochasticUnshadowedShadingOutput);
    }
     
    void DenoiserPreBlurRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SeparableBlur);

        BlurTexture(context, ResourceNames::StochasticShadowedShadingOutput);
        BlurTexture(context, ResourceNames::StochasticUnshadowedShadingOutput);
    }

    void DenoiserPreBlurRenderPass::BlurTexture(RenderContext<RenderPassContentMediator>* context, Foundation::Name textureName)
    {
        auto resourceProvider = context->GetResourceProvider();

        Geometry::Dimensions dispatchDimensions = resourceProvider->GetTextureProperties(textureName).Dimensions;

        BlurCBContent cbContent{};
        cbContent.BlurRadius = 5;
        cbContent.IsHorizontal = true;
        cbContent.Weights.fill(1.0f / cbContent.BlurRadius);
        auto kernel = Foundation::Gaussian::Kernel1D(cbContent.BlurRadius);
        std::move(kernel.begin(), kernel.end(), cbContent.Weights.begin());
        cbContent.ImageSize = { dispatchDimensions.Width, dispatchDimensions.Height };

        cbContent.InputTexIdx = resourceProvider->GetUATextureIndex(textureName);
        cbContent.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoisedPreBlurIntermediate);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(dispatchDimensions, { 256, 1 });

        // Blur vertical
        std::swap(dispatchDimensions.Width, dispatchDimensions.Height);
        std::swap(cbContent.InputTexIdx, cbContent.OutputTexIdx);
        cbContent.IsHorizontal = false;

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(dispatchDimensions, { 256, 1 });
    }

}
