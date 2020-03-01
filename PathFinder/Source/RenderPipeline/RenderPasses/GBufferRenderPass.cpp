#include "GBufferRenderPass.hpp"

namespace PathFinder
{

    GBufferRenderPass::GBufferRenderPass()
        : RenderPass("GBuffer") {}

    void GBufferRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature GBufferMeshesSinature = stateCreator->CloneBaseRootSignature();
        GBufferMeshesSinature.AddConstantsParameter(HAL::RootConstantsParameter{ 1, 0, 0 }); // Mesh instance table index
        GBufferMeshesSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 }); // Unified vertex buffer
        GBufferMeshesSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 1, 0 }); // Unified index buffer
        GBufferMeshesSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 2, 0 }); // Instance data buffer
        GBufferMeshesSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 3, 0 }); // Material data buffer
        stateCreator->StoreRootSignature(RootSignatureNames::GBufferMeshes, std::move(GBufferMeshesSinature));

        HAL::RootSignature GBufferLightsSinature = stateCreator->CloneBaseRootSignature();
        GBufferLightsSinature.AddConstantsParameter(HAL::RootConstantsParameter{ 1, 0, 0 }); // Lights table index
        GBufferLightsSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 }); // Lights table
        stateCreator->StoreRootSignature(RootSignatureNames::GBufferLights, std::move(GBufferLightsSinature));

        stateCreator->CreateGraphicsState(PSONames::GBufferMeshes, [](GraphicsStateProxy& state) 
        {
            state.ShaderFileNames.VertexShaderFileName = L"GBufferMeshes.hlsl";
            state.ShaderFileNames.PixelShaderFileName = L"GBufferMeshes.hlsl";
            state.RenderTargetFormats = { HAL::ColorFormat::RGBA32_Unsigned };
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::GBufferMeshes;
            state.DepthStencilState.SetDepthTestEnabled(true);
        });

        stateCreator->CreateGraphicsState(PSONames::GBufferLights, [](GraphicsStateProxy& state)
        {
            state.ShaderFileNames.VertexShaderFileName = L"GBufferLights.hlsl";
            state.ShaderFileNames.PixelShaderFileName = L"GBufferLights.hlsl";
            state.RenderTargetFormats = { HAL::ColorFormat::RGBA32_Unsigned };
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::GBufferLights;
            state.DepthStencilState.SetDepthTestEnabled(true);
        });
    }
      
    void GBufferRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        ResourceScheduler::NewTextureProperties RT0Properties{};
        RT0Properties.ShaderVisibleFormat = HAL::ColorFormat::RGBA32_Unsigned;

        scheduler->NewRenderTarget(ResourceNames::GBufferRT0, RT0Properties);
        scheduler->NewDepthStencil(ResourceNames::GBufferDepthStencil); 
    }  

    void GBufferRenderPass::Render(RenderContext<RenderPassContentMediator>* context) 
    {
        context->GetCommandRecorder()->SetRenderTargetAndDepthStencil(ResourceNames::GBufferRT0, ResourceNames::GBufferDepthStencil);
        context->GetCommandRecorder()->ClearRenderTarget(ResourceNames::GBufferRT0, Foundation::Color::Black());
        context->GetCommandRecorder()->ClearDepth(ResourceNames::GBufferDepthStencil, 1.0f);

        RenderMeshes(context);
        RenderLights(context);
    }

    void GBufferRenderPass::RenderMeshes(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GBufferMeshes);

        // Use vertex and index buffers as normal structured buffers
        auto meshStorage = context->GetContent()->GetSceneGPUStorage();
        context->GetCommandRecorder()->BindExternalBuffer(*meshStorage->UnifiedVertexBuffer(), 0, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*meshStorage->UnifiedIndexBuffer(), 1, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*meshStorage->MeshInstanceTable(), 2, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*meshStorage->MaterialTable(), 3, 0, HAL::ShaderRegister::ShaderResource);

        for (const MeshInstance& instance : context->GetContent()->GetScene()->MeshInstances())
        {
            context->GetCommandRecorder()->SetRootConstants(instance.GPUInstanceIndex(), 0, 0);
            context->GetCommandRecorder()->Draw(instance.AssosiatedMesh()->LocationInVertexStorage().IndexCount);
        }
    }

    void GBufferRenderPass::RenderLights(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GBufferLights);

        // Use vertex and index buffers as normal structured buffers
        auto lightStorage = context->GetContent()->GetSceneGPUStorage();
        context->GetCommandRecorder()->BindExternalBuffer(*lightStorage->LightTable(), 0, 0, HAL::ShaderRegister::ShaderResource);

        for (const FlatLight& light : context->GetContent()->GetScene()->FlatLights())
        {
            context->GetCommandRecorder()->SetRootConstants(light.GPULightTableIndex(), 0, 0);
            context->GetCommandRecorder()->Draw(6); // Light is a rotated billboard: 2 triangles from 6 vertices
        }

        for (const SphericalLight& light : context->GetContent()->GetScene()->SphericalLights())
        {
            context->GetCommandRecorder()->SetRootConstants(light.GPULightTableIndex(), 0, 0);
            context->GetCommandRecorder()->Draw(6); // Light is a rotated billboard: 2 triangles from 6 vertices
        }
    }

}
