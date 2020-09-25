#include "DenoiserPostBlurRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserPostBlurRenderPass::DenoiserPostBlurRenderPass()
        : RenderPass("DenoiserPostBlur") {}

    void DenoiserPostBlurRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserPostBlur, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserPostBlur.hlsl";
        });
    }

    void DenoiserPostBlurRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto frameIndex = scheduler->FrameNumber() % 2;

        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingPostBlurred);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingPostBlurred);
        scheduler->NewTexture(ResourceNames::CombinedShading);

        ResourceScheduler::NewTextureProperties oversaturatedProps{};
        oversaturatedProps.MipCount = 3;
        scheduler->NewTexture(ResourceNames::CombinedShadingOverexposed, oversaturatedProps);

        scheduler->ReadTexture(ResourceNames::ShadingAnalyticOutput);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingDenoised[frameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingDenoised[frameIndex]);
        scheduler->ReadTexture(ResourceNames::DenoiserReprojectedFramesCount[frameIndex]);
        scheduler->ReadTexture(ResourceNames::DenoiserSecondaryGradient);
    }
     
    void DenoiserPostBlurRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserPostBlur);

        auto resourceProvider = context->GetResourceProvider();
        auto previousFrameIndex = (context->FrameNumber() - 1) % 2;
        auto frameIndex = context->FrameNumber() % 2;

        auto groupCount = CommandRecorder::DispatchGroupCount(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });

        const RenderSettings* settings = context->GetContent()->GetSettings();

        DenoiserPostBlurCBContent cbContent{};
        cbContent.DispatchGroupCount = { groupCount.Width, groupCount.Height };
        cbContent.AnalyticShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::ShadingAnalyticOutput);
        cbContent.SecondaryGradientTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserSecondaryGradient);
        cbContent.AccumulatedFramesCountTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserReprojectedFramesCount[frameIndex]);
        cbContent.ShadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingDenoised[frameIndex]);
        cbContent.UnshadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingDenoised[frameIndex]);
        cbContent.ShadowedShadingBlurredOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingPostBlurred);
        cbContent.UnshadowedShadingBlurredOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingPostBlurred);
        cbContent.CombinedShadingTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::CombinedShading);
        cbContent.CombinedShadingOversaturatedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::CombinedShadingOverexposed);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(groupCount.Width, groupCount.Height);
    }

}
