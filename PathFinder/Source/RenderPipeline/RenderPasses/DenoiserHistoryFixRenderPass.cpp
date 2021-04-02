#include "DenoiserHistoryFixRenderPass.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    DenoiserHistoryFixRenderPass::DenoiserHistoryFixRenderPass()
        : RenderPass("DenoiserHistoryFix") {}

    void DenoiserHistoryFixRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserHistoryFix, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserHistoryFix.hlsl";
        });
    }

    void DenoiserHistoryFixRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        if (!scheduler->Content()->GetSettings()->IsDenoiserEnabled)
            return;

        auto frameIndex = scheduler->FrameNumber() % 2;

        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferViewDepth[frameIndex]);
        scheduler->ReadTexture(ResourceNames::DenoiserReprojectedFramesCount[frameIndex]);
        
        // Write to mip 0
        scheduler->AliasAndWriteTexture(ResourceNames::StochasticShadowedShadingPreBlurred, ResourceNames::StochasticShadowedShadingFixed, MipSet::FirstMip());
        scheduler->AliasAndWriteTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred, ResourceNames::StochasticUnshadowedShadingFixed, MipSet::FirstMip());

        // Read tail mips
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingPreBlurred, MipSet::Range(1, std::nullopt));
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred, MipSet::Range(1, std::nullopt));
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
        cbContent.ShadowedShadingPreBlurredTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingPreBlurred, 1);
        cbContent.UnshadowedShadingPreBlurredTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingPreBlurred, 1);
        cbContent.ShadowedShadingFixedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingFixed);
        cbContent.UnshadowedShadingFixedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingFixed);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
