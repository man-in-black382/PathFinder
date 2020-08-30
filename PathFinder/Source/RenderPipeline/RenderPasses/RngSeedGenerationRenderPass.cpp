#include "RngSeedGenerationRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    RngSeedGenerationRenderPass::RngSeedGenerationRenderPass()
        : RenderPass("RngSeedGeneration") {}

    void RngSeedGenerationRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::RngSeedGeneration, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "RngSeedGeneration.hlsl";
        });
    }

    void RngSeedGenerationRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto currentFrameIndex = scheduler->FrameNumber() % 2;
        auto previousFrameIndex = (scheduler->FrameNumber() - 1) % 2;

        ResourceScheduler::NewTextureProperties rngSeedsProperties{ HAL::ColorFormat::RGBA8_Unsigned };
        rngSeedsProperties.Flags = ResourceScheduler::Flags::CrossFrameRead;

        scheduler->NewTexture(ResourceNames::RngSeeds[currentFrameIndex], rngSeedsProperties);
        scheduler->NewTexture(ResourceNames::RngSeeds[previousFrameIndex], ResourceScheduler::MipSet::Empty(), rngSeedsProperties);

        //scheduler->ExecuteOnQueue(RenderPassExecutionQueue::AsyncCompute);
    }
     
    void RngSeedGenerationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::RngSeedGeneration);

        auto resourceProvider = context->GetResourceProvider();
        auto frameIndex = context->FrameNumber() % 2;

        const Scene* scene = context->GetContent()->GetScene();
        const Memory::Texture* blueNoiseTexture = scene->BlueNoiseTexture();

        RngSeedGenerationCBContent cbContent{};
        cbContent.RngSeedTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::RngSeeds[frameIndex]);
        cbContent.FrameNumber = context->FrameNumber();
        cbContent.BlueNoiseTexSize = blueNoiseTexture->Properties().Dimensions.Width; // W = H
        cbContent.BlueNoiseTexDepth = blueNoiseTexture->Properties().Dimensions.Depth;

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
