#include "PlaygroundRenderPass.hpp"

namespace PathFinder
{

    PlaygroundRenderPass::PlaygroundRenderPass()
        : RenderPass("Playground") {}

    void PlaygroundRenderPass::SetupPipelineStates(IShaderManager* shaderManager, IPipelineStateManager* psoManager)
    {
        auto pso = psoManager->CloneDefaultGraphicsState();
        pso.SetShaders(shaderManager->LoadShaders("Playground.hlsl", "Playground.hlsl"));
        pso.SetInputAssemblerLayout(CommonInputAssemblerLayouts::Layout1P3());
        pso.SetDepthStencilFormat(HAL::ResourceFormat::Depth24_Float_Stencil8_Unsigned);
        pso.SetRenderTargetFormats(HAL::ResourceFormat::Color::RGBA8_Usigned_Norm);
        pso.SetPrimitiveTopology(HAL::PrimitiveTopology::TriangleList);
        psoManager->StoreGraphicsState(PSONames::GBuffer, pso);
    }

    void PlaygroundRenderPass::ScheduleResources(IResourceScheduler* scheduler)
    {
        scheduler->WillRenderToDepthStencil(ResourceNames::MainDepthStencil);
    }

    void PlaygroundRenderPass::Render(IGraphicsDevice* device)
    {
        device->ApplyPipelineState(PSONames::GBuffer);
        device->SetBackBufferAsRenderTarget(ResourceNames::MainDepthStencil);
        device->ClearBackBuffer(Foundation::Color::Green());
    }

}
