#include "TAARenderPass.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    TAARenderPass::TAARenderPass()
        : RenderPass("TAA") {}

    void TAARenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::TAA, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "TAA.hlsl";
        });
    }

    void TAARenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        auto previousFrameIndex = (scheduler->GetFrameNumber() - 1) % 2;
        auto frameIndex = scheduler->GetFrameNumber() % 2;

        NewTextureProperties properties{};
        properties.Flags = ResourceSchedulingFlags::CrossFrameRead;

        scheduler->NewTexture(ResourceNames::TAAOutput[frameIndex], properties);
        scheduler->NewTexture(ResourceNames::TAAOutput[previousFrameIndex], MipSet::Empty(), properties);

        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::SMAAAntialiased);
        scheduler->ReadTexture(ResourceNames::TAAOutput[previousFrameIndex]);
    }
     
    void TAARenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::TAA);

        auto resourceProvider = context->GetResourceProvider();
        auto previousFrameIndex = (context->GetFrameNumber() - 1) % 2;
        auto frameIndex = context->GetFrameNumber() % 2;

        auto groupCount = CommandRecorder::DispatchGroupCount(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });

        TAACBContent cbContent{};
        cbContent.DispatchGroupCount = { groupCount.Width, groupCount.Height };
        cbContent.MotionTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferMotionVector);
        cbContent.CurrentFrameTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::SMAAAntialiased);
        cbContent.PreviousFrameTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::TAAOutput[previousFrameIndex]);
        cbContent.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::TAAOutput[frameIndex]);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(groupCount.Width, groupCount.Height);
    }

}
