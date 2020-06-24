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
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::GBufferMeshes;
            state.DepthStencilState.SetDepthTestEnabled(true);
            state.RenderTargetFormats = {
                HAL::ColorFormat::RGBA8_Usigned_Norm,
                HAL::ColorFormat::RGBA8_Usigned_Norm,
                HAL::ColorFormat::R32_Unsigned,
                HAL::ColorFormat::R8_Unsigned,
                HAL::ColorFormat::R32_Float
            };
        });

        stateCreator->CreateGraphicsState(PSONames::GBufferLights, [](GraphicsStateProxy& state)
        {
            state.VertexShaderFileName = "GBufferLights.hlsl";
            state.PixelShaderFileName = "GBufferLights.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::GBufferLights;
            state.DepthStencilState.SetDepthTestEnabled(true);
            state.RenderTargetFormats = {
                HAL::ColorFormat::RGBA8_Usigned_Norm,
                HAL::ColorFormat::RGBA8_Usigned_Norm,
                HAL::ColorFormat::R32_Unsigned,
                HAL::ColorFormat::R8_Unsigned,
                HAL::ColorFormat::R32_Float
            };
        });
    }
      
    void GBufferRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        ResourceScheduler::NewTextureProperties albedoMetalnessProperties{ HAL::ColorFormat::RGBA8_Usigned_Norm };
        ResourceScheduler::NewTextureProperties normalRoughnessProperties{ HAL::ColorFormat::RGBA8_Usigned_Norm };
        ResourceScheduler::NewTextureProperties motionVectorProperties{ HAL::ColorFormat::R32_Unsigned };
        ResourceScheduler::NewTextureProperties typeAndMaterialIndexProperties{ HAL::ColorFormat::R8_Unsigned };

        ResourceScheduler::NewTextureProperties viewDepthProperties{};
        viewDepthProperties.ShaderVisibleFormat = HAL::ColorFormat::R32_Float;
        viewDepthProperties.MipCount = 5; 
        viewDepthProperties.ClearValues = HAL::ColorClearValue{ std::numeric_limits<float>::max() };
        viewDepthProperties.Flags = ResourceScheduler::Flags::CrossFrameRead;

        scheduler->NewRenderTarget(ResourceNames::GBufferAlbedoMetalness, albedoMetalnessProperties);
        scheduler->NewRenderTarget(ResourceNames::GBufferNormalRoughness, normalRoughnessProperties);
        scheduler->NewRenderTarget(ResourceNames::GBufferMotionVector, motionVectorProperties);
        scheduler->NewRenderTarget(ResourceNames::GBufferTypeAndMaterialIndex, typeAndMaterialIndexProperties);
        scheduler->NewRenderTarget(ResourceNames::GBufferViewDepth[0], viewDepthProperties);
        scheduler->NewRenderTarget(ResourceNames::GBufferViewDepth[1], viewDepthProperties);
        scheduler->NewDepthStencil(ResourceNames::GBufferDepthStencil);
    }  

    void GBufferRenderPass::Render(RenderContext<RenderPassContentMediator>* context) 
    {
        auto currentFrameIndex = context->FrameNumber() % 2;

        context->GetCommandRecorder()->SetRenderTargets(
            std::array{
                ResourceNames::GBufferAlbedoMetalness,
                ResourceNames::GBufferNormalRoughness,
                ResourceNames::GBufferMotionVector,
                ResourceNames::GBufferTypeAndMaterialIndex,
                ResourceNames::GBufferViewDepth[currentFrameIndex]
            },
            ResourceNames::GBufferDepthStencil);

        context->GetCommandRecorder()->ClearRenderTarget(ResourceNames::GBufferAlbedoMetalness);
        context->GetCommandRecorder()->ClearRenderTarget(ResourceNames::GBufferNormalRoughness);
        context->GetCommandRecorder()->ClearRenderTarget(ResourceNames::GBufferMotionVector);
        context->GetCommandRecorder()->ClearRenderTarget(ResourceNames::GBufferTypeAndMaterialIndex);
        context->GetCommandRecorder()->ClearRenderTarget(ResourceNames::GBufferViewDepth[currentFrameIndex]);
        context->GetCommandRecorder()->ClearDepth(ResourceNames::GBufferDepthStencil);

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
