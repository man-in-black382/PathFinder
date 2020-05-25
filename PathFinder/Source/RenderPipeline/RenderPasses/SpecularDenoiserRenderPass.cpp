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
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingReprojectedHistoryArrayIdx });
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingReprojectedHistoryArrayIdx });
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx });
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx });
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingDenoiseTargetArrayIdx });
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingDenoiseTargetArrayIdx });
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
        cbContent.CurrentShadowedShadingTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx });
        cbContent.CurrentUnshadowedShadingTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx });
        cbContent.ShadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingReprojectedHistoryArrayIdx });
        cbContent.UnshadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingReprojectedHistoryArrayIdx });
        cbContent.ShadowedShadingDenoiseTargetTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingDenoiseTargetArrayIdx });
        cbContent.UnshadowedShadingDenoiseTargetTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingDenoiseTargetArrayIdx });

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
