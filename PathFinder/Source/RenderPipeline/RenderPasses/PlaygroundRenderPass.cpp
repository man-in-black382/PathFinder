#include "PlaygroundRenderPass.hpp"

namespace PathFinder
{

    PlaygroundRenderPass::PlaygroundRenderPass()
        : RenderPass("Playground") {}

    void PlaygroundRenderPass::SetupPipelineStates(IShaderManager* shaderManager, IPipelineStateManager* psoManager)
    {
        auto rootSig = psoManager->CloneBaseRootSignature();
        psoManager->StoreRootSignature(RootSignatureNames::Universal, rootSig);

        auto pso = psoManager->CloneDefaultGraphicsState();
        pso.SetShaders(shaderManager->LoadShaders("Playground.hlsl", "Playground.hlsl")); 
        pso.SetInputAssemblerLayout(InputAssemblerLayoutForVertexLayout(VertexLayout::Layout1P1N1UV1T1BT));
        pso.SetDepthStencilFormat(HAL::ResourceFormat::Depth24_Float_Stencil8_Unsigned);
        pso.SetRenderTargetFormats(HAL::ResourceFormat::Color::RGBA8_Usigned_Norm);
        pso.SetPrimitiveTopology(HAL::PrimitiveTopology::TriangleList); 
        psoManager->StoreGraphicsState(PSONames::GBuffer, pso, RootSignatureNames::Universal); 
    }
      
    void PlaygroundRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->NewRenderTarget(ResourceNames::PlaygroundRenderTarget);
        scheduler->NewDepthStencil(ResourceNames::GBufferDepthStencil);
        scheduler->WillUseRootConstantBuffer<PlaygroundCBContent>();
    }  

    void PlaygroundRenderPass::Render(RenderContext* context) 
    {
        context->GetGraphicsDevice()->ApplyPipelineState(PSONames::GBuffer);
        //context->GetGraphicsDevice()->SetBackBufferAsRenderTarget(ResourceNames::GBufferDepthStencil);
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
