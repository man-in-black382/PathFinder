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
        if (!scheduler->GetContent()->GetSettings()->IsDenoiserEnabled)
            return;

        auto frameIndex = scheduler->GetFrameNumber() % 2;

        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferViewDepth[frameIndex]);
        scheduler->ReadTexture(ResourceNames::ReprojectedFramesCount[frameIndex]);
        
        // Write to mip 0
        scheduler->AliasAndWriteTexture(ResourceNames::StochasticShadowedShadingPreBlurred, ResourceNames::StochasticShadowedShadingFixed, MipSet::FirstMip());
        scheduler->AliasAndWriteTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred, ResourceNames::StochasticUnshadowedShadingFixed, MipSet::FirstMip());

        // Read tail mips
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingPreBlurred, TextureReadContext::NonPixelShader, MipSet::Range(1, std::nullopt));
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred, TextureReadContext::NonPixelShader, MipSet::Range(1, std::nullopt));
    }
     
    void DenoiserHistoryFixRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserHistoryFix);

        auto resourceProvider = context->GetResourceProvider();

        auto previousFrameIndex = (context->GetFrameNumber() - 1) % 2;
        auto frameIndex = context->GetFrameNumber() % 2;

        DenoiserHistoryFixCBContent cbContent{};
        cbContent.GBufferNormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughness);
        cbContent.ViewDepthTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferViewDepth[frameIndex]);
        cbContent.AccumulationCounterTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::ReprojectedFramesCount[frameIndex]);
        cbContent.ShadowedShadingPreBlurredTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingPreBlurred, 1);
        cbContent.UnshadowedShadingPreBlurredTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingPreBlurred, 1);
        cbContent.ShadowedShadingFixedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingFixed);
        cbContent.UnshadowedShadingFixedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingFixed);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
