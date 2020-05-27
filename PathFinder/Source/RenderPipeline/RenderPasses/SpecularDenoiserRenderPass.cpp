#include "SpecularDenoiserRenderPass.hpp"
#include "DownsamplingCBContent.hpp"

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
        auto currentFrameIndex = scheduler->FrameNumber() % 2;

        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
        scheduler->ReadTexture({ ResourceNames::GBufferViewDepth, currentFrameIndex });
        scheduler->ReadTexture({ ResourceNames::DenoiserReprojectedFramesCount, currentFrameIndex });
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingReprojected);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingReprojected);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingOutput);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingOutput);
        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingDenoised);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingDenoised);
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
        cbContent.GBufferIndices.ViewDepthTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::GBufferViewDepth, currentFrameIndex });
        cbContent.AccumulatedFramesCountTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::DenoiserReprojectedFramesCount, currentFrameIndex });
        cbContent.CurrentShadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingOutput);
        cbContent.CurrentUnshadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingOutput);
        cbContent.ShadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingReprojected);
        cbContent.UnshadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingReprojected);
        cbContent.ShadowedShadingDenoiseTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingDenoised);
        cbContent.UnshadowedShadingDenoiseTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingDenoised);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
