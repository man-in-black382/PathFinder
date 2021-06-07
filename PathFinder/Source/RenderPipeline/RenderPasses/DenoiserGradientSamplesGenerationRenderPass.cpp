#include "DenoiserGradientSamplesGenerationRenderPass.hpp"
#include "UAVClearHelper.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    DenoiserGradientSamplesGenerationRenderPass::DenoiserGradientSamplesGenerationRenderPass()
        : RenderPass("DenoiserGradientSamplesGeneration") {}

    void DenoiserGradientSamplesGenerationRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserGradientSamplesGeneration, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserGradientSamplesGeneration.hlsl";
        });
    }

    void DenoiserGradientSamplesGenerationRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        if (!scheduler->GetContent()->GetSettings()->IsDenoiserEnabled)
            return;

        auto currentFrameIndex = scheduler->GetFrameNumber() % 2;
        auto previousFrameIndex = (scheduler->GetFrameNumber() - 1) % 2;

        Geometry::Dimensions gradientTexturesSize = scheduler->GetDefaultRenderSurfaceDesc().Dimensions().XYMultiplied(1.0 / 3.0);

        NewTextureProperties gradientSamplePositionsProps{ HAL::ColorFormat::R8_Unsigned, HAL::TextureKind::Texture2D, gradientTexturesSize };
        NewTextureProperties gradientProps{ HAL::ColorFormat::RG16_Float, HAL::TextureKind::Texture2D, gradientTexturesSize };

        gradientSamplePositionsProps.Flags = ResourceSchedulingFlags::CrossFrameRead;

        scheduler->NewTexture(ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex], gradientSamplePositionsProps);
        scheduler->NewTexture(ResourceNames::DenoiserGradientSamplePositions[previousFrameIndex], MipSet::Empty(), gradientSamplePositionsProps);
        scheduler->NewTexture(ResourceNames::DenoiserGradientSamples, gradientProps);

        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
        scheduler->ReadTexture(ResourceNames::DenoisedReprojectedTexelIndices);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingOutput[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::RngSeeds[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::DenoiserGradientSamplePositions[previousFrameIndex]);

        scheduler->AliasAndWriteTexture(ResourceNames::RngSeeds[currentFrameIndex], ResourceNames::RngSeedsCorrelated);

        scheduler->ExecuteOnQueue(RenderPassExecutionQueue::AsyncCompute);
    }
     
    void DenoiserGradientSamplesGenerationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        auto resourceProvider = context->GetResourceProvider();
        auto currentFrameIndex = context->GetFrameNumber() % 2;
        auto previousFrameIndex = (context->GetFrameNumber() - 1) % 2;

        ClearUAVTextureFloat(context, ResourceNames::DenoiserGradientSamples, glm::vec4{ -1.0f });
        ClearUAVTextureUInt(context, ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex], glm::uvec4{ 0 });

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserGradientSamplesGeneration);

        auto groupCount = CommandRecorder::DispatchGroupCount(resourceProvider->GetTextureProperties(ResourceNames::DenoiserGradientSamples).Dimensions, { 8, 8 });

        DenoiserGradientSamplesGenerationCBContent cbContent{};
        cbContent.DispatchGroupCount = { groupCount.Width, groupCount.Height };
        cbContent.DepthStencilTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        cbContent.ReprojectedTexelIndicesTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoisedReprojectedTexelIndices);
        cbContent.StochasticRngSeedsPrevTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::RngSeeds[previousFrameIndex]);
        cbContent.GradientSamplePositionsPrevTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserGradientSamplePositions[previousFrameIndex]);
        cbContent.StochasticRngSeedsOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::RngSeedsCorrelated);
        cbContent.GradientSamplePositionsOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex]);
        cbContent.GradientSamplesOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoiserGradientSamples);
        cbContent.FrameNumber = context->GetFrameNumber();

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(groupCount.Width, groupCount.Height);
    }

}
