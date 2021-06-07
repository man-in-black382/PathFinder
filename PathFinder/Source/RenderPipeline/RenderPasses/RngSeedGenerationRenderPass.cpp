#include "RngSeedGenerationRenderPass.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    RngSeedGenerationRenderPass::RngSeedGenerationRenderPass()
        : RenderPass("RngSeedGeneration") {}

    void RngSeedGenerationRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::RngSeedGeneration, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "RngSeedGeneration.hlsl";
        });
    }

    void RngSeedGenerationRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        auto currentFrameIndex = scheduler->GetFrameNumber() % 2;
        auto previousFrameIndex = (scheduler->GetFrameNumber() - 1) % 2;

        NewTextureProperties rngSeedsProperties{ HAL::ColorFormat::RGBA8_Unsigned };
        rngSeedsProperties.Flags = ResourceSchedulingFlags::CrossFrameRead;

        scheduler->NewTexture(ResourceNames::RngSeeds[currentFrameIndex], rngSeedsProperties);
        scheduler->NewTexture(ResourceNames::RngSeeds[previousFrameIndex], MipSet::Empty(), rngSeedsProperties);

        scheduler->ExecuteOnQueue(RenderPassExecutionQueue::AsyncCompute);
    }
     
    void RngSeedGenerationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::RngSeedGeneration);

        auto resourceProvider = context->GetResourceProvider();
        auto frameIndex = context->GetFrameNumber() % 2;

        const Scene* scene = context->GetContent()->GetScene();
        const Memory::Texture* blueNoiseTexture = scene->GetBlueNoiseTexture();

        RngSeedGenerationCBContent cbContent{};
        cbContent.RngSeedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::RngSeeds[frameIndex]);
        cbContent.FrameNumber = context->GetFrameNumber();
        cbContent.BlueNoiseTexSize = blueNoiseTexture->Properties().Dimensions.Width; // W = H
        cbContent.BlueNoiseTexDepth = blueNoiseTexture->Properties().Dimensions.Depth;

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 8, 8 });
    }

}
