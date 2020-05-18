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

        // Read previous frame reprojected denoised lighting history
        auto reprojectedHistoryTexIdx = 2u;
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticShadowedOutput, reprojectedHistoryTexIdx });
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, reprojectedHistoryTexIdx });

        // Read current frame lighting, denoise
        auto currentFrameLightingTexIdx = 0u;
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticShadowedOutput, currentFrameLightingTexIdx });
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, currentFrameLightingTexIdx });

        // Use 2nd texture as denoiser target
        auto denoiseTargetTexIdx = 1u;
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticShadowedOutput, denoiseTargetTexIdx });
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, denoiseTargetTexIdx });
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

        cbContent.CurrentShadowedShadingTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, 0 });
        cbContent.CurrentUnshadowedShadingTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, 0 });

        cbContent.ShadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, 2 });
        cbContent.UnshadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, 2 });

        cbContent.ShadowedShadingDenoiseTargetTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, 1 });
        cbContent.UnshadowedShadingDenoiseTargetTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, 1 });

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
