#include "GBufferRenderPass.hpp"

namespace PathFinder
{

    GBufferRenderPass::GBufferRenderPass()
        : RenderPass("GBuffer") {}

    void GBufferRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        rootSignatureCreator->CreateRootSignature(RootSignatureNames::GBufferMeshes, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddRootConstantsParameter<uint32_t>(0, 0); // Mesh instance table index
            signatureProxy.AddShaderResourceBufferParameter(0, 0); // Unified vertex buffer
            signatureProxy.AddShaderResourceBufferParameter(1, 0); // Unified index buffer
            signatureProxy.AddShaderResourceBufferParameter(2, 0); // Instance data buffer
            signatureProxy.AddShaderResourceBufferParameter(3, 0); // Material data buffer
        });

        rootSignatureCreator->CreateRootSignature(RootSignatureNames::GBufferLights, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddRootConstantsParameter<uint32_t>(0, 0); // Lights table index
            signatureProxy.AddShaderResourceBufferParameter(0, 0); // Lights table
        });

        stateCreator->CreateGraphicsState(PSONames::GBufferMeshes, [](GraphicsStateProxy& state) 
        {
            state.VertexShaderFileName = "GBufferMeshes.hlsl";
            state.PixelShaderFileName = "GBufferMeshes.hlsl";
            state.RenderTargetFormats = { HAL::ColorFormat::RGBA32_Unsigned, HAL::ColorFormat::R32_Float };
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::GBufferMeshes;
            state.DepthStencilState.SetDepthTestEnabled(true);
        });

        stateCreator->CreateGraphicsState(PSONames::GBufferLights, [](GraphicsStateProxy& state)
        {
            state.VertexShaderFileName = "GBufferLights.hlsl";
            state.PixelShaderFileName = "GBufferLights.hlsl";
            state.RenderTargetFormats = { HAL::ColorFormat::RGBA32_Unsigned, HAL::ColorFormat::R32_Float };
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::GBufferLights;
            state.DepthStencilState.SetDepthTestEnabled(true);
        });
    }
      
    void GBufferRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        ResourceScheduler::NewTextureProperties RT0Properties{};
        RT0Properties.ShaderVisibleFormat = HAL::ColorFormat::RGBA32_Unsigned;

        ResourceScheduler::NewTextureProperties viewDepthProperties{};
        viewDepthProperties.ShaderVisibleFormat = HAL::ColorFormat::R32_Float;
        viewDepthProperties.TextureCount = 2; // 2 for reprojection
        viewDepthProperties.MipCount = 5; 

        scheduler->NewRenderTarget(ResourceNames::GBufferRT0, RT0Properties);
        scheduler->NewRenderTarget(ResourceNames::GBufferViewDepth, viewDepthProperties);
        scheduler->NewDepthStencil(ResourceNames::GBufferDepthStencil);
    }  

    void GBufferRenderPass::Render(RenderContext<RenderPassContentMediator>* context) 
    {
        auto textureIndex = context->FrameNumber() % 2;

        context->GetCommandRecorder()->SetRenderTargets(
            std::array{ ResourceKey{ResourceNames::GBufferRT0}, ResourceKey{ResourceNames::GBufferViewDepth, textureIndex} },
            ResourceKey{ResourceNames::GBufferDepthStencil});

        context->GetCommandRecorder()->ClearRenderTarget(ResourceNames::GBufferRT0, Foundation::Color::Black());
        context->GetCommandRecorder()->ClearRenderTarget({ ResourceNames::GBufferViewDepth, textureIndex }, Foundation::Color::Black());
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

        for (const FlatLight& light : context->GetContent()->GetScene()->RectangularLights())
        {
            context->GetCommandRecorder()->SetRootConstants(light.GPULightTableIndex(), 0, 0);
            context->GetCommandRecorder()->Draw(6); // Light is a rotated billboard: 2 triangles from 6 vertices
        }

        for (const FlatLight& light : context->GetContent()->GetScene()->DiskLights())
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
