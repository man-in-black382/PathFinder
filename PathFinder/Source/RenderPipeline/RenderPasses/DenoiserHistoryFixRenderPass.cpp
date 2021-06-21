#include "DenoiserHistoryFixRenderPass.hpp"
#include "ResourceNameResolving.hpp"

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

        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughnessPatched);
        scheduler->ReadTexture(ResourceNames::GBufferViewDepthPatched);
        scheduler->ReadTexture(ResourceNames::ReprojectedFramesCount[frameIndex]);
        
        scheduler->ReadTexture(DenoiserHistoryFixShadowedInputTexName(false, frameIndex), TextureReadContext::NonPixelShader, MipSet::AllMips());
        scheduler->ReadTexture(DenoiserHistoryFixUnshadowedInputTexName(false), TextureReadContext::NonPixelShader, MipSet::AllMips());

        NewTextureProperties shadowedShadingFixedProperties{};
        shadowedShadingFixedProperties.TextureToCopyPropertiesFrom = DenoiserHistoryFixShadowedInputTexName(false, frameIndex);
        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingFixed, shadowedShadingFixedProperties);

        NewTextureProperties unshadowedShadingFixedProperties{};
        unshadowedShadingFixedProperties.TextureToCopyPropertiesFrom = DenoiserHistoryFixUnshadowedInputTexName(false);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingFixed, unshadowedShadingFixedProperties);
    }
     
    void DenoiserHistoryFixRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserHistoryFix);

        auto resourceProvider = context->GetResourceProvider();

        auto previousFrameIndex = (context->GetFrameNumber() - 1) % 2;
        auto frameIndex = context->GetFrameNumber() % 2;

        DenoiserHistoryFixCBContent cbContent{};
        cbContent.GBufferNormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughnessPatched);
        cbContent.ViewDepthTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferViewDepthPatched);
        cbContent.AccumulationCounterTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::ReprojectedFramesCount[frameIndex]);
        cbContent.ShadowedShadingPreBlurredTexIdx = resourceProvider->GetSRTextureIndex(DenoiserHistoryFixShadowedInputTexName(false, frameIndex));
        cbContent.UnshadowedShadingPreBlurredTexIdx = resourceProvider->GetSRTextureIndex(DenoiserHistoryFixUnshadowedInputTexName(false));
        cbContent.ShadowedShadingFixedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingFixed);
        cbContent.UnshadowedShadingFixedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingFixed);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 8, 8 });
    }

}
