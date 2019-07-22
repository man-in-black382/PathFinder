#include "PlaygroundRenderPass.hpp"

namespace PathFinder
{

    PlaygroundRenderPass::PlaygroundRenderPass()
        : RenderPass(FrameResourceNames::PlaygroundRenderTarget, "Playground.hlsl", "Playground.hlsl") {}


    void PlaygroundRenderPass::ScheduleResources(IResourceScheduler* scheduler)
    {
        //scheduler->WillRenderToRenderTarget(FrameResourceNames::PlaygroundRenderTarget);
    }

    void PlaygroundRenderPass::Render(IResourceProvider* resourceProvider, GraphicsDevice* device)
    {
        auto rtView = resourceProvider->GetBackBuffer();
        device->SetRenderTarget(rtView);
        device->ClearRenderTarget(Foundation::Color::Green(), rtView);
    }

}
