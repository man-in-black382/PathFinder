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

        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingOutput);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingOutput);
        scheduler->ReadWriteTexture(ResourceNames::StochasticShadowedShadingOutput, { 0 });
        scheduler->ReadWriteTexture(ResourceNames::StochasticUnshadowedShadingOutput, { 0 });
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
        cbContent.ShadowedShadingMip0TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingOutput, 0);
        cbContent.UnshadowedShadingMip0TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingOutput, 0);
        cbContent.ShadowedShadingTailMipsTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingOutput, 1);
        cbContent.UnshadowedShadingTailMipsTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingOutput, 1);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
