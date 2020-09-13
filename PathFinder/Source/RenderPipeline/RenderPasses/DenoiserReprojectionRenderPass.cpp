#include "DenoiserReprojectionRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserReprojectionRenderPass::DenoiserReprojectionRenderPass()
        : RenderPass("DenoiserReprojection") {}

    void DenoiserReprojectionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserReprojection, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserReprojection.hlsl";
        });
    }

    void DenoiserReprojectionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto previousFrameIndex = (scheduler->FrameNumber() - 1) % 2;
        auto frameIndex = scheduler->FrameNumber() % 2;

        ResourceScheduler::NewTextureProperties frameCountProperties{};
        frameCountProperties.ShaderVisibleFormat = HAL::ColorFormat::R16_Float;
        frameCountProperties.Flags = ResourceScheduler::Flags::CrossFrameRead;

        scheduler->NewTexture(ResourceNames::DenoiserReprojectedFramesCount[frameIndex], frameCountProperties);
        scheduler->NewTexture(ResourceNames::DenoiserReprojectedFramesCount[previousFrameIndex], ResourceScheduler::MipSet::Empty(), frameCountProperties);
        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingReprojected);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingReprojected);

        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::GBufferViewDepth[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::GBufferViewDepth[frameIndex]);
        scheduler->ReadTexture(ResourceNames::DenoiserReprojectedFramesCount[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingDenoised[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingDenoised[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingPreBlurred);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred);
    }
     
    void DenoiserReprojectionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserReprojection);

        auto resourceProvider = context->GetResourceProvider();

        auto previousFrameIndex = (context->FrameNumber() - 1) % 2;
        auto frameIndex = context->FrameNumber() % 2;

        auto groupCount = CommandRecorder::DispatchGroupCount(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });

        DenoiserReprojectionCBContent cbContent{};
        cbContent.DispatchGroupCount = { groupCount.Width, groupCount.Height };
        cbContent.GBufferNormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughness);
        cbContent.DepthTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        cbContent.MotionTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferMotionVector);
        cbContent.PreviousViewDepthTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferViewDepth[previousFrameIndex]);
        cbContent.CurrentViewDepthTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferViewDepth[frameIndex]);
        cbContent.PreviousAccumulationCounterTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserReprojectedFramesCount[previousFrameIndex]);
        cbContent.CurrentAccumulationCounterTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoiserReprojectedFramesCount[frameIndex]);
        cbContent.ShadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingDenoised[previousFrameIndex]);
        cbContent.UnshadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingDenoised[previousFrameIndex]);
        cbContent.ShadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingPreBlurred);
        cbContent.UnshadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingPreBlurred);
        cbContent.ShadowedShadingReprojectionTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingReprojected);
        cbContent.UnshadowedShadingReprojectionTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingReprojected);
        //cbContent.ShadingGradientTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadingGradient);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(groupCount.Width, groupCount.Height);
    }

}
