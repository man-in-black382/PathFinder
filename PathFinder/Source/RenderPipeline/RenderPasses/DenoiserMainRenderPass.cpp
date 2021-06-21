#include "DenoiserMainRenderPass.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    DenoiserMainRenderPass::DenoiserMainRenderPass()
        : RenderPass("DenoiserMainPass") {}

    void DenoiserMainRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserMainPass, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserMainPass.hlsl";
        });
    }

    void DenoiserMainRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        if (!scheduler->GetContent()->GetSettings()->IsDenoiserEnabled)
            return;

        auto previousFrameIndex = (scheduler->GetFrameNumber() - 1) % 2;
        auto currentFrameIndex = scheduler->GetFrameNumber() % 2;

        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughnessPatched);
        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
        //scheduler->ReadTexture(ResourceNames::ReprojectedFramesCount[currentFrameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingReprojected);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingReprojected);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingFixed);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingFixed);
        scheduler->ReadTexture(ResourceNames::DenoiserGradientFiltered);

        NewTextureProperties outputProperties{};
        outputProperties.Flags = ResourceSchedulingFlags::CrossFrameRead;
        outputProperties.TextureToCopyPropertiesFrom = ResourceNames::StochasticShadowedShadingOutput[currentFrameIndex]; // Same format for shadowed and unshadowed

        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingDenoised[currentFrameIndex], outputProperties);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingDenoised[currentFrameIndex], outputProperties);
        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingDenoised[previousFrameIndex], MipSet::Empty(), outputProperties);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingDenoised[previousFrameIndex], MipSet::Empty(), outputProperties);
        scheduler->NewTexture(ResourceNames::DenoiserSecondaryGradient, NewTextureProperties{ HAL::ColorFormat::RG8_Usigned_Norm });

        scheduler->AliasAndWriteTexture(ResourceNames::ReprojectedFramesCount[currentFrameIndex], ResourceNames::ReprojectedFramesCountPatched);
    }
     
    void DenoiserMainRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserMainPass);

        auto resourceProvider = context->GetResourceProvider();
        auto previousFrameIndex = (context->GetFrameNumber() - 1) % 2;
        auto currentFrameIndex = context->GetFrameNumber() % 2;

        auto groupCount = CommandRecorder::DispatchGroupCount(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 8, 8 });

        const RenderSettings* settings = context->GetContent()->GetSettings();

        SpecularDenoiserCBContent cbContent{};

        cbContent.GBufferIndices.NormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughnessPatched);
        cbContent.GBufferIndices.MotionTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferMotionVector);
        cbContent.GBufferIndices.DepthStencilTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        cbContent.DispatchGroupCount = { groupCount.Width, groupCount.Height };
        //cbContent.AccumulatedFramesCountTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::ReprojectedFramesCount[currentFrameIndex]);
        cbContent.CurrentShadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingFixed);
        cbContent.CurrentUnshadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingFixed);
        cbContent.ShadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingReprojected);
        cbContent.UnshadowedShadingHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingReprojected);
        cbContent.ShadowedShadingDenoiseTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingDenoised[currentFrameIndex]);
        cbContent.UnshadowedShadingDenoiseTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingDenoised[currentFrameIndex]);
        cbContent.AccumulatedFramesCountPatchedTargetTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ReprojectedFramesCountPatched);
        cbContent.PrimaryGradientTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserGradientFiltered);
        cbContent.SecondaryGradientTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoiserSecondaryGradient);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(groupCount.Width, groupCount.Height);
    }

}
