#include "DenoiserReprojectionRenderPass.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    DenoiserReprojectionRenderPass::DenoiserReprojectionRenderPass()
        : RenderPass("DenoiserReprojection") {}

    void DenoiserReprojectionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserReprojection, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserReprojection.hlsl";
        });
    }

    void DenoiserReprojectionRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        auto previousFrameIndex = (scheduler->GetFrameNumber() - 1) % 2;
        auto frameIndex = scheduler->GetFrameNumber() % 2;

        NewTextureProperties frameCountProperties{};
        frameCountProperties.ShaderVisibleFormat = HAL::ColorFormat::R16_Float;
        frameCountProperties.Flags = ResourceSchedulingFlags::CrossFrameRead;

        NewTextureProperties reprojectionTargetProperties{};
        reprojectionTargetProperties.TextureToCopyPropertiesFrom = ResourceNames::StochasticShadowedShadingOutput[frameIndex];

        scheduler->NewTexture(ResourceNames::ReprojectedFramesCount[frameIndex], frameCountProperties);
        scheduler->NewTexture(ResourceNames::ReprojectedFramesCount[previousFrameIndex], MipSet::Empty(), frameCountProperties);

        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::GBufferViewDepth[previousFrameIndex], TextureReadContext::NonPixelShader, MipSet::FirstMip());
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil, TextureReadContext::NonPixelShader, MipSet::FirstMip());
        scheduler->ReadTexture(ResourceNames::ReprojectedFramesCount[previousFrameIndex]);

        if (scheduler->GetContent()->GetSettings()->IsDenoiserEnabled)
        {
            scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingOutput[previousFrameIndex]);
            scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingDenoised[previousFrameIndex]);
            scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingDenoised[previousFrameIndex]);

            scheduler->NewTexture(ResourceNames::StochasticShadowedShadingReprojected, reprojectionTargetProperties);
            scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingReprojected, reprojectionTargetProperties);
        }
    }
     
    void DenoiserReprojectionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserReprojection);

        auto resourceProvider = context->GetResourceProvider();
        auto previousFrameIndex = (context->GetFrameNumber() - 1) % 2;
        auto frameIndex = context->GetFrameNumber() % 2;
        auto groupCount = CommandRecorder::DispatchGroupCount(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 8, 8 });

        DenoiserReprojectionCBContent cbContent{};
        cbContent.DispatchGroupCount = { groupCount.Width, groupCount.Height };
        cbContent.GBufferNormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughness);
        cbContent.MotionTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferMotionVector);
        cbContent.PreviousViewDepthTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferViewDepth[previousFrameIndex]);
        cbContent.DepthStencilTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        cbContent.PreviousAccumulationCounterTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::ReprojectedFramesCount[previousFrameIndex]);
        cbContent.CurrentAccumulationCounterTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ReprojectedFramesCount[frameIndex]);

        if (context->GetContent()->GetSettings()->IsDenoiserEnabled)
        {
            cbContent.ShadowedShadingDenoisedHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingDenoised[previousFrameIndex]);
            cbContent.UnshadowedShadingDenoisedHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingDenoised[previousFrameIndex]);
            cbContent.ShadowedShadingReprojectionTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingReprojected);
            cbContent.UnshadowedShadingReprojectionTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingReprojected);
        }

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(groupCount.Width, groupCount.Height);
    }

}
