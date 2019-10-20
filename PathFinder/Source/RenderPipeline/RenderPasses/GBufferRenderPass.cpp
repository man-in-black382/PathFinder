#include "GBufferRenderPass.hpp"

namespace PathFinder
{

    GBufferRenderPass::GBufferRenderPass()
        : RenderPass("GBuffer") {}

    void GBufferRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature GBufferSinature = stateCreator->CloneBaseRootSignature();
        GBufferSinature.AddDescriptorParameter({ 0, 0 });
        stateCreator->StoreRootSignature(RootSignatureNames::GBuffer, std::move(GBufferSinature));

        stateCreator->CreateGraphicsState(PSONames::GBuffer, [](GraphicsStateProxy& state)
        {
            state.ShaderFileNames.VertexShaderFileName = L"GBuffer.hlsl";
            state.ShaderFileNames.PixelShaderFileName = L"GBuffer.hlsl";
            state.InputLayout = InputAssemblerLayoutForVertexLayout(VertexLayout::Layout1P1N1UV1T1BT);
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::GBuffer;
        });
    }
      
    void GBufferRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->NewRenderTarget(ResourceNames::GBufferRenderTarget);
        scheduler->NewDepthStencil(ResourceNames::GBufferDepthStencil);
        scheduler->WillUseRootConstantBuffer<GBufferCBContent>();
    }  

    void GBufferRenderPass::Render(RenderContext* context) 
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GBuffer);
        context->GetCommandRecorder()->SetRenderTargetAndDepthStencil(ResourceNames::GBufferRenderTarget, ResourceNames::GBufferDepthStencil);
        context->GetCommandRecorder()->ClearBackBuffer(Foundation::Color::Gray());
        context->GetCommandRecorder()->ClearDepth(ResourceNames::GBufferDepthStencil, 1.0f);
        context->GetCommandRecorder()->UseVertexBufferOfLayout(VertexLayout::Layout1P1N1UV1T1BT);
        context->GetCommandRecorder()->BindMeshInstanceTableConstantBuffer(0);

        context->GetScene()->IterateMeshInstances([&](const MeshInstance& instance)
        {
            context->GetScene()->IterateSubMeshes(*instance.AssosiatedMesh(), [&](const SubMesh& subMesh)
            {
                context->GetCommandRecorder()->Draw(subMesh.LocationInVertexStorage());
            });
        });
    }

}
