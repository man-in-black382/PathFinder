#include "PlaygroundRenderPass.hpp"

namespace PathFinder
{

    PlaygroundRenderPass::PlaygroundRenderPass()
        : RenderPass("Playground") {}

    void PlaygroundRenderPass::SetupPipelineStates(IShaderManager* shaderManager, IPipelineStateManager* psoManager)
    {
        auto pso = psoManager->CloneDefaultGraphicsState();
        pso.SetShaders(shaderManager->LoadShaders("Playground.hlsl", "Playground.hlsl")); 
        pso.SetInputAssemblerLayout(InputAssemblerLayoutForVertexLayout(VertexLayout::Layout1P1N1UV1T1BT));
        pso.SetDepthStencilFormat(HAL::ResourceFormat::Depth24_Float_Stencil8_Unsigned);
        pso.SetRenderTargetFormats(HAL::ResourceFormat::Color::RGBA8_Usigned_Norm);
        pso.SetPrimitiveTopology(HAL::PrimitiveTopology::TriangleList);
        psoManager->StoreGraphicsState(PSONames::GBuffer, pso);
    }

    void PlaygroundRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->WillRenderToRenderTarget(ResourceNames::PlaygroundRenderTarget);
        scheduler->WillRenderToDepthStencil(ResourceNames::GBufferDepthStencil);
        scheduler->WillUseRootConstantBuffer<PlaygroundCBContent>();
    }

    void PlaygroundRenderPass::Render(RenderContext* context)
    {
        context->GraphicsDevice()->ApplyPipelineState(PSONames::GBuffer);
        context->GraphicsDevice()->SetRenderTargetAndDepthStencil(ResourceNames::PlaygroundRenderTarget, ResourceNames::GBufferDepthStencil);
        context->GraphicsDevice()->ClearBackBuffer(Foundation::Color::Gray());
        context->GraphicsDevice()->ClearDepth(ResourceNames::GBufferDepthStencil, 1.0f);
        context->GraphicsDevice()->UseVertexBufferOfLayout(VertexLayout::Layout1P1N1UV1T1BT);
        context->GraphicsDevice()->SetViewport({ 1280, 720 });

        auto cbContent = context->ConstantsUpdater()->UpdateRootConstantBuffer<PlaygroundCBContent>();
        cbContent->cameraMat = context->World()->MainCamera().ViewProjection();

        context->World()->IterateMeshInstances([&](const MeshInstance& instance)
        {
            context->World()->IterateSubMeshes(*instance.AssosiatedMesh(), [&](const SubMesh& subMesh)
            {
                context->GraphicsDevice()->Draw(subMesh.LocationInVertexStorage());
            });
        });
    }

}
