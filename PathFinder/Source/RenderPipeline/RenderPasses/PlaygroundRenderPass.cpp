#include "PlaygroundRenderPass.hpp"

namespace PathFinder
{

    PlaygroundRenderPass::PlaygroundRenderPass()
        : RenderPass("Playground") {}

    void PlaygroundRenderPass::SetupPipelineStates(IShaderManager* shaderManager, IPipelineStateManager* psoManager)
    {
        auto pso = psoManager->CloneDefaultGraphicsState();
        pso.SetShaders(shaderManager->LoadShaders("Playground.hlsl", "Playground.hlsl"));
        psoManager->StoreGraphicsState(PSONames::GBuffer, pso);
    }

    void PlaygroundRenderPass::ScheduleResources(IResourceScheduler* scheduler)
    {
        //scheduler->WillRenderToRenderTarget(FrameResourceNames::PlaygroundRenderTarget);
    }

    void PlaygroundRenderPass::Render(IResourceProvider* resourceProvider, IGraphicsDevice* device)
    {
        device->ApplyPipelineState(PSONames::GBuffer);
        //auto rtView = resourceProvider->GetBackBuffer();
        device->SetBackBufferAsRenderTarget();
        device->ClearBackBuffer(Foundation::Color::Green());
    }

}
