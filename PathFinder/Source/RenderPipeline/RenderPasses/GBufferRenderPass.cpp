#include "GBufferRenderPass.hpp"

namespace PathFinder
{

    GBufferRenderPass::GBufferRenderPass()
        : RenderPass("GBuffer") {}

    void GBufferRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature GBufferSinature = stateCreator->CloneBaseRootSignature();
        GBufferSinature.AddConstantsParameter(HAL::RootConstantsParameter{ 1, 0, 0 }); // 1 uint index
        GBufferSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 }); // Unified vertex buffer
        GBufferSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 1, 0 }); // Unified index buffer
        GBufferSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 2, 0 }); // Instance data buffer
        stateCreator->StoreRootSignature(RootSignatureNames::GBuffer, std::move(GBufferSinature));

        stateCreator->CreateGraphicsState(PSONames::GBuffer, [](GraphicsStateProxy& state)
        {
            state.ShaderFileNames.VertexShaderFileName = L"GBufferRenderPass.hlsl";
            state.ShaderFileNames.PixelShaderFileName = L"GBufferRenderPass.hlsl";
            state.RenderTargetFormats = { HAL::ResourceFormat::Color::RGBA32_Unsigned };
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::GBuffer;
            state.DepthStencilState.SetDepthTestEnabled(true);
        });
    }
      
    void GBufferRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        ResourceScheduler::NewTextureProperties RT0Properties{};
        RT0Properties.ShaderVisibleFormat = HAL::ResourceFormat::Color::RGBA32_Unsigned;

        scheduler->NewRenderTarget(ResourceNames::GBufferRT0, RT0Properties);
        scheduler->NewDepthStencil(ResourceNames::GBufferDepthStencil);
    }  

    void GBufferRenderPass::Render(RenderContext* context) 
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GBuffer);
        context->GetCommandRecorder()->SetRenderTargetAndDepthStencil(ResourceNames::GBufferRT0, ResourceNames::GBufferDepthStencil);
        context->GetCommandRecorder()->ClearRenderTarget(ResourceNames::GBufferRT0, Foundation::Color::Black());
        context->GetCommandRecorder()->ClearDepth(ResourceNames::GBufferDepthStencil, 1.0f);

        // Use vertex and index buffers as normal structured buffers
        context->GetCommandRecorder()->BindExternalBuffer(*context->GetMeshStorage()->UnifiedVertexBuffer_1P1N1UV1T1BT(), 0, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*context->GetMeshStorage()->UnifiedIndexBuffer_1P1N1UV1T1BT(), 1, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(context->GetMeshStorage()->InstanceTable(), 2, 0, HAL::ShaderRegister::ShaderResource);

        context->GetScene()->IterateMeshInstances([&](const MeshInstance& instance)
        {
            context->GetCommandRecorder()->SetRootConstants(instance.GPUInstanceIndex(), 0, 0);
            context->GetCommandRecorder()->Draw(instance.AssosiatedMesh()->LocationInVertexStorage().IndexCount);
        });
    }

}
