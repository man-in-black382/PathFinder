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
        frameCountProperties.TextureCount = 2;

        scheduler->NewTexture(ResourceNames::DenoiserReprojectedFramesCount, frameCountProperties);

        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
        scheduler->ReadTexture({ ResourceNames::GBufferViewDepth, previousFrameIndex });
        scheduler->ReadTexture({ ResourceNames::GBufferViewDepth, frameIndex });
        scheduler->ReadTexture({ ResourceNames::DenoiserReprojectedFramesCount, previousFrameIndex });
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingHistoryArrayIdx });
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingHistoryArrayIdx });
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingReprojectedHistoryArrayIdx });
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingReprojectedHistoryArrayIdx });
    }
     
    void DenoiserReprojectionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserReprojection);

        auto resourceProvider = context->GetResourceProvider();

        auto previousFrameIndex = (context->FrameNumber() - 1) % 2;
        auto frameIndex = context->FrameNumber() % 2;

        DenoiserReprojectionCBContent cbContent{};
        cbContent.GBufferNormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughness);
        cbContent.DepthTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        cbContent.PreviousViewDepthTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::GBufferViewDepth, previousFrameIndex });
        cbContent.CurrentViewDepthTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::GBufferViewDepth, frameIndex });
        cbContent.PreviousAccumulationCounterTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::DenoiserReprojectedFramesCount, previousFrameIndex });
        cbContent.CurrentAccumulationCounterTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::DenoiserReprojectedFramesCount, frameIndex });
        cbContent.ShadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingHistoryArrayIdx });
        cbContent.UnshadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingHistoryArrayIdx });
        cbContent.ShadowedShadingReprojectionTargetTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingReprojectedHistoryArrayIdx });
        cbContent.UnshadowedShadingReprojectionTargetTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingReprojectedHistoryArrayIdx });

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
