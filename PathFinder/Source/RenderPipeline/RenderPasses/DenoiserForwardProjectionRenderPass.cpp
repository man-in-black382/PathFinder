#include "DenoiserForwardProjectionRenderPass.hpp"
#include "UAVClearHelper.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    DenoiserForwardProjectionRenderPass::DenoiserForwardProjectionRenderPass()
        : RenderPass("DenoiserForwardProjection") {}

    void DenoiserForwardProjectionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserForwardProjection, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserForwardProjection.hlsl";
        });
    }

    void DenoiserForwardProjectionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto currentFrameIndex = scheduler->FrameNumber() % 2;
        auto previousFrameIndex = (scheduler->FrameNumber() - 1) % 2;

        Geometry::Dimensions gradientTexturesSize = scheduler->DefaultRenderSurfaceDesc().Dimensions().XYMultiplied(1.0 / 3.0);

        ResourceScheduler::NewTextureProperties gradientSamplePositionsProps{ HAL::ColorFormat::R8_Unsigned, HAL::TextureKind::Texture2D, gradientTexturesSize };
        ResourceScheduler::NewTextureProperties gradientProps{ HAL::ColorFormat::RG16_Float, HAL::TextureKind::Texture2D, gradientTexturesSize };

        gradientSamplePositionsProps.Flags = ResourceScheduler::Flags::CrossFrameRead;

        scheduler->NewTexture(ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex], gradientSamplePositionsProps);
        scheduler->NewTexture(ResourceNames::DenoiserGradientSamplePositions[previousFrameIndex], ResourceScheduler::MipSet::Empty(), gradientSamplePositionsProps);
        scheduler->NewTexture(ResourceNames::DenoiserPrimaryGradientInputs, gradientProps);

        scheduler->ReadTexture(ResourceNames::GBufferViewDepth[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingDenoised[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingDenoised[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::RngSeeds[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::DenoiserGradientSamplePositions[previousFrameIndex]);

        scheduler->AliasAndWriteTexture(ResourceNames::RngSeeds[currentFrameIndex], ResourceNames::RngSeedsCorrelated);
    }
     
    void DenoiserForwardProjectionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        auto resourceProvider = context->GetResourceProvider();
        auto currentFrameIndex = context->FrameNumber() % 2;
        auto previousFrameIndex = (context->FrameNumber() - 1) % 2;

        ClearUAVTextureFloat(context, ResourceNames::DenoiserPrimaryGradientInputs, glm::vec4{ -1.0f });
        ClearUAVTextureUInt(context, ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex], glm::uvec4{ 0 });

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserForwardProjection);

        auto groupCount = CommandRecorder::DispatchGroupCount(resourceProvider->GetTextureProperties(ResourceNames::DenoiserPrimaryGradientInputs).Dimensions, { 16, 16 });

        DenoiserForwardProjectionCBContent cbContent{};
        cbContent.DispatchGroupCount = { groupCount.Width, groupCount.Height };
        cbContent.GBufferViewDepthPrevTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferViewDepth[previousFrameIndex]);
        cbContent.StochasticShadowedShadingPrevTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingDenoised[previousFrameIndex]);
        cbContent.StochasticUnshadowedShadingPrevTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingDenoised[previousFrameIndex]);
        cbContent.StochasticRngSeedsPrevTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::RngSeeds[previousFrameIndex]);
        cbContent.GradientSamplePositionsPrevTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserGradientSamplePositions[previousFrameIndex]);
        cbContent.StochasticRngSeedsTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::RngSeedsCorrelated);
        cbContent.GradientSamplePositionsTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex]);
        cbContent.GradientTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoiserPrimaryGradientInputs);
        cbContent.FrameNumber = context->FrameNumber();

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(groupCount.Width, groupCount.Height);
    }

}
