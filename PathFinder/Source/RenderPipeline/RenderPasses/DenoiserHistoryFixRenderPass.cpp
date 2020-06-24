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
        scheduler->ReadTexture(ResourceNames::GBufferViewDepth[frameIndex]);
        scheduler->ReadTexture(ResourceNames::DenoiserReprojectedFramesCount[frameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingPreBlurred);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred);

        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingFixed);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingFixed);
    }
     
    void DenoiserHistoryFixRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserHistoryFix);

        auto resourceProvider = context->GetResourceProvider();

        auto previousFrameIndex = (context->FrameNumber() - 1) % 2;
        auto frameIndex = context->FrameNumber() % 2;

        DenoiserHistoryFixCBContent cbContent{};
        cbContent.GBufferNormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughness);
        cbContent.ViewDepthTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferViewDepth[frameIndex]);
        cbContent.AccumulationCounterTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserReprojectedFramesCount[frameIndex]);
        cbContent.ShadowedShadingFixedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingFixed);
        cbContent.UnshadowedShadingFixedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingFixed);
        cbContent.ShadowedShadingPreBlurredTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingPreBlurred);
        cbContent.UnshadowedShadingPreBlurredTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingPreBlurred);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
