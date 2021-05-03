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
        bool isDenoiserEnabled = scheduler->GetContent()->GetSettings()->IsDenoiserEnabled;
        auto currentFrameIndex = scheduler->GetFrameNumber() % 2;

        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingPostBlurred);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingPostBlurred);

        NewTextureProperties combinedShadingProps{};
        combinedShadingProps.MipCount = NewTextureProperties::FullMipChain;
        scheduler->NewTexture(ResourceNames::CombinedShading, combinedShadingProps);

        NewTextureProperties oversaturatedProps{};
        oversaturatedProps.MipCount = 3;
        scheduler->NewTexture(ResourceNames::CombinedShadingOversaturated, MipSet::FirstMip(), oversaturatedProps);

        scheduler->ReadTexture(ResourceNames::ShadingAnalyticOutput);
        scheduler->ReadTexture(DenoiserPostBlurStochasticShadowedInputTexName(isDenoiserEnabled, currentFrameIndex));
        scheduler->ReadTexture(DenoiserPostBlurStochasticUnshadowedInputTexName(isDenoiserEnabled, currentFrameIndex));
        scheduler->ReadTexture(ResourceNames::GIIrradianceProbeAtlas[currentFrameIndex]);
        scheduler->ReadTexture(ResourceNames::GIDepthProbeAtlas[currentFrameIndex]);
        
        scheduler->ReadTexture(ResourceNames::GBufferAlbedoMetalness);
        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::GBufferTypeAndMaterialIndex);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);

        if (isDenoiserEnabled)
        {
            scheduler->ReadTexture(ResourceNames::ReprojectedFramesCount[currentFrameIndex]);
            scheduler->ReadTexture(ResourceNames::DenoiserSecondaryGradient);
        }
    }
     
    void DenoiserPostBlurRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        bool isDenoiserEnabled = context->GetContent()->GetSettings()->IsDenoiserEnabled;

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserPostBlur);

        auto resourceProvider = context->GetResourceProvider();
        auto previousFrameIndex = (context->GetFrameNumber() - 1) % 2;
        auto frameIndex = context->GetFrameNumber() % 2;

        auto groupCount = CommandRecorder::DispatchGroupCount(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });

        const RenderSettings* settings = context->GetContent()->GetSettings();
        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();

        DenoiserPostBlurCBContent cbContent{};

        cbContent.ProbeField = sceneStorage->GetIrradianceFieldGPURepresentation();
        cbContent.ProbeField.CurrentIrradianceProbeAtlasTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIIrradianceProbeAtlas[frameIndex]);
        cbContent.ProbeField.CurrentDepthProbeAtlasTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIDepthProbeAtlas[frameIndex]);

        cbContent.GBufferIndices.AlbedoMetalnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferAlbedoMetalness);
        cbContent.GBufferIndices.NormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughness);
        cbContent.GBufferIndices.MotionTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferMotionVector);
        cbContent.GBufferIndices.TypeAndMaterialTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferTypeAndMaterialIndex);
        cbContent.GBufferIndices.DepthStencilTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);

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
            cbContent.AccumulatedFramesCountTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::ReprojectedFramesCount[frameIndex]);
        }

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(groupCount.Width, groupCount.Height);
    }

}
