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
        //scheduler->NewTexture("1", ResourceScheduler::NewTextureProperties{ std::nullopt, Geometry::Dimensions{100, 100}, std::nullopt, std::nullopt });
        //scheduler->NewTexture("2", ResourceScheduler::NewTextureProperties{ std::nullopt,  Geometry::Dimensions{200, 100}, std::nullopt, std::nullopt });
        //scheduler->NewTexture("100", ResourceScheduler::NewTextureProperties{ std::nullopt, Geometry::Dimensions{100, 100}, std::nullopt, std::nullopt });
        //scheduler->NewTexture("101", ResourceScheduler::NewTextureProperties{ std::nullopt, Geometry::Dimensions{100, 100}, std::nullopt, std::nullopt });
        //scheduler->NewTexture("102", ResourceScheduler::NewTextureProperties{ std::nullopt, Geometry::Dimensions{100, 100}, std::nullopt, std::nullopt });
          scheduler->NewDepthStencil(ResourceNames::GBufferDepthStencil);
          scheduler->WillUseRootConstantBuffer<GBufferCBContent>();
    }  

    void GBufferRenderPass::Render(RenderContext* context) 
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GBuffer);
        context->GetCommandRecorder()->SetRenderTargetAndDepthStencil(ResourceNames::PlaygroundRenderTarget, ResourceNames::GBufferDepthStencil);
        context->GetCommandRecorder()->ClearBackBuffer(Foundation::Color::Gray());
        context->GetCommandRecorder()->ClearDepth(ResourceNames::GBufferDepthStencil, 1.0f);
        context->GetCommandRecorder()->UseVertexBufferOfLayout(VertexLayout::Layout1P1N1UV1T1BT);

        context->GetScene()->IterateMeshInstances([&](const MeshInstance& instance)
        {
            context->GetScene()->IterateSubMeshes(*instance.AssosiatedMesh(), [&](const SubMesh& subMesh)
            {
                context->GetCommandRecorder()->Draw(subMesh.LocationInVertexStorage());
            });
        });
    }

}
