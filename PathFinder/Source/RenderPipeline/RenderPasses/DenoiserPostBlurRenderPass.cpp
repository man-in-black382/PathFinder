#include "DenoiserPostBlurRenderPass.hpp"
#include "DownsamplingHelper.hpp"
#include "ResourceNameResolving.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    DenoiserPostBlurRenderPass::DenoiserPostBlurRenderPass()
        : RenderPass("DenoiserPostBlur") {}

    void DenoiserPostBlurRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserPostBlur, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserPostBlur.hlsl";
        });
    }

    void DenoiserPostBlurRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        bool isDenoiserEnabled = scheduler->Content()->GetSettings()->IsDenoiserEnabled;

        auto frameIndex = scheduler->FrameNumber() % 2;

        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingPostBlurred);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingPostBlurred);

        NewTextureProperties combinedShadingProps{};
        combinedShadingProps.MipCount = NewTextureProperties::FullMipChain;
        scheduler->NewTexture(ResourceNames::CombinedShading, combinedShadingProps);

        NewTextureProperties oversaturatedProps{};
        oversaturatedProps.MipCount = 3;
        scheduler->NewTexture(ResourceNames::CombinedShadingOversaturated, oversaturatedProps);

        scheduler->ReadTexture(ResourceNames::ShadingAnalyticOutput);
        scheduler->ReadTexture(DenoiserPostBlurStochasticShadowedInputTexName(isDenoiserEnabled, frameIndex)); 
        scheduler->ReadTexture(DenoiserPostBlurStochasticUnshadowedInputTexName(isDenoiserEnabled, frameIndex));

        if (isDenoiserEnabled)
        {
            scheduler->ReadTexture(ResourceNames::DenoiserReprojectedFramesCount[frameIndex]);
            scheduler->ReadTexture(ResourceNames::DenoiserSecondaryGradient);
        }
    }
     
    void DenoiserPostBlurRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        bool isDenoiserEnabled = context->GetContent()->GetSettings()->IsDenoiserEnabled;

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserPostBlur);

        auto resourceProvider = context->GetResourceProvider();
        auto previousFrameIndex = (context->FrameNumber() - 1) % 2;
        auto frameIndex = context->FrameNumber() % 2;

        auto groupCount = CommandRecorder::DispatchGroupCount(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });

        const RenderSettings* settings = context->GetContent()->GetSettings();

        DenoiserPostBlurCBContent cbContent{};
        cbContent.DispatchGroupCount = { groupCount.Width, groupCount.Height };
        cbContent.AnalyticShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::ShadingAnalyticOutput);
        cbContent.ShadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(DenoiserPostBlurStochasticShadowedInputTexName(isDenoiserEnabled, frameIndex));
        cbContent.UnshadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(DenoiserPostBlurStochasticUnshadowedInputTexName(isDenoiserEnabled, frameIndex));
        cbContent.ShadowedShadingBlurredOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingPostBlurred);
        cbContent.UnshadowedShadingBlurredOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingPostBlurred);
        cbContent.CombinedShadingTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::CombinedShading);
        cbContent.CombinedShadingOversaturatedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::CombinedShadingOversaturated);

        if (isDenoiserEnabled)
        {
            cbContent.SecondaryGradientTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserSecondaryGradient);
            cbContent.AccumulatedFramesCountTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserReprojectedFramesCount[frameIndex]);
        }

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(groupCount.Width, groupCount.Height);
    }

}
