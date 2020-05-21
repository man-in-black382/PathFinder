#include "DenoiserHistoryFixRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserHistoryFixRenderPass::DenoiserHistoryFixRenderPass()
        : RenderPass("DenoiserHistoryFix") {}

    void DenoiserHistoryFixRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserHistoryFix, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserHistoryFix.hlsl";
        });
    }

    void DenoiserHistoryFixRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto frameIndex = scheduler->FrameNumber() % 2;

        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture({ ResourceNames::GBufferViewDepth, frameIndex });
        scheduler->ReadTexture({ ResourceNames::DenoiserReprojectedFramesCount, frameIndex });

        auto currentFrameShadingTexIdx = 0u;
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticShadowedOutput, currentFrameShadingTexIdx });
        scheduler->ReadTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, currentFrameShadingTexIdx });
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticShadowedOutput, currentFrameShadingTexIdx }, { 0 });
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, currentFrameShadingTexIdx }, { 0 });
    }
     
    void DenoiserHistoryFixRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserHistoryFix);

        auto resourceProvider = context->GetResourceProvider();

        auto previousFrameIndex = (context->FrameNumber() - 1) % 2;
        auto frameIndex = context->FrameNumber() % 2;

        DenoiserHistoryFixCBContent cbContent{};
        cbContent.GBufferNormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughness);
        cbContent.ViewDepthTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::GBufferViewDepth, frameIndex });
        cbContent.AccumulationCounterTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::DenoiserReprojectedFramesCount, frameIndex });
        cbContent.ShadowedShadingMip0TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, 0 }, 0);
        cbContent.UnshadowedShadingMip0TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, 0 }, 0);
        cbContent.ShadowedShadingTailMipsTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, 0 }, 1);
        cbContent.UnshadowedShadingTailMipsTexIdx = resourceProvider->GetSRTextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, 0 }, 1);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
