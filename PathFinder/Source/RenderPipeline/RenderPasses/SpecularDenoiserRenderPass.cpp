#include "SpecularDenoiserRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    SpecularDenoiserRenderPass::SpecularDenoiserRenderPass()
        : RenderPass("SpecularDenoiser") {}

    void SpecularDenoiserRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::SpecularDenoiser, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "SpecularDenoiser.hlsl";
        });
    }

    void SpecularDenoiserRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto previousFrameIndex = (scheduler->FrameNumber() - 1) % 2;
        auto currentFrameIndex = scheduler->FrameNumber() % 2;

        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
        scheduler->ReadTexture(ResourceNames::GBufferViewDepth[currentFrameIndex]);
        scheduler->ReadTexture(ResourceNames::DenoiserReprojectedFramesCount[currentFrameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingReprojected);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingReprojected);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingFixed);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingFixed);

        ResourceScheduler::NewTextureProperties outputProperties{};
        outputProperties.Flags = ResourceScheduler::Flags::CrossFrameRead;

        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingDenoised[currentFrameIndex], outputProperties);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingDenoised[currentFrameIndex], outputProperties);
        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingDenoised[previousFrameIndex], ResourceScheduler::MipSet::Empty(), outputProperties);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingDenoised[previousFrameIndex], ResourceScheduler::MipSet::Empty(), outputProperties);
    }
     
    void SpecularDenoiserRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SpecularDenoiser);

        auto resourceProvider = context->GetResourceProvider();
        auto currentFrameIndex = context->FrameNumber() % 2;

        SpecularDenoiserCBContent cbContent{};

        cbContent.GBufferIndices.NormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughness);
        cbContent.GBufferIndices.MotionTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferMotionVector);
        cbContent.GBufferIndices.DepthStencilTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        cbContent.GBufferIndices.ViewDepthTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferViewDepth[currentFrameIndex]);
        cbContent.AccumulatedFramesCountTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserReprojectedFramesCount[currentFrameIndex]);
        cbContent.CurrentShadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingFixed);
        cbContent.CurrentUnshadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingFixed);
        cbContent.ShadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingReprojected);
        cbContent.UnshadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingReprojected);
        cbContent.ShadowedShadingDenoiseTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingDenoised[currentFrameIndex]);
        cbContent.UnshadowedShadingDenoiseTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingDenoised[currentFrameIndex]);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
