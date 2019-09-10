#include "GBufferRenderPass.hpp"

namespace PathFinder
{

    GBufferRenderPass::GBufferRenderPass()
        : RenderPass("GBuffer") {}

    void GBufferRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateGraphicsState(PSONames::GBuffer, [](GraphicsStateProxy& state)
        {
            state.ShaderFileNames.VertexShaderFileName = L"Playground.hlsl";
            state.ShaderFileNames.PixelShaderFileName = L"Playground.hlsl";
            state.InputLayout = InputAssemblerLayoutForVertexLayout(VertexLayout::Layout1P1N1UV1T1BT);
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
        });
    }
      
    void GBufferRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->NewRenderTarget(ResourceNames::PlaygroundRenderTarget);
        scheduler->NewDepthStencil(ResourceNames::GBufferDepthStencil);
        scheduler->WillUseRootConstantBuffer<PlaygroundCBContent>();
    }  

    void GBufferRenderPass::Render(RenderContext* context) 
    {
        context->GetGraphicsDevice()->ApplyPipelineState(PSONames::GBuffer);
        context->GetGraphicsDevice()->SetRenderTargetAndDepthStencil(ResourceNames::PlaygroundRenderTarget, ResourceNames::GBufferDepthStencil);
        context->GetGraphicsDevice()->ClearBackBuffer(Foundation::Color::Gray());
        context->GetGraphicsDevice()->ClearDepth(ResourceNames::GBufferDepthStencil, 1.0f);
        context->GetGraphicsDevice()->UseVertexBufferOfLayout(VertexLayout::Layout1P1N1UV1T1BT);
        context->GetGraphicsDevice()->SetViewport({ 1280, 720 });

        context->GetScene()->IterateMeshInstances([&](const MeshInstance& instance)
        {
            context->GetScene()->IterateSubMeshes(*instance.AssosiatedMesh(), [&](const SubMesh& subMesh)
            {
                context->GetGraphicsDevice()->Draw(subMesh.LocationInVertexStorage());
            });
        });
    }

}
