#include "GBufferRenderPass.hpp"

namespace PathFinder
{

    GBufferRenderPass::GBufferRenderPass()
        : RenderPass("GBuffer") {}

    void GBufferRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature GBufferSinature = stateCreator->CloneBaseRootSignature();
        GBufferSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 });
        GBufferSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 1, 0 });
        GBufferSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 2, 0 });
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

        ResourceScheduler::NewTextureProperties parallaxCounterProperties{};
        parallaxCounterProperties.ShaderVisibleFormat = HAL::ResourceFormat::Color::R16_Unsigned;

        scheduler->NewTexture("POMCounter", parallaxCounterProperties);
        scheduler->NewRenderTarget(ResourceNames::GBufferRT0, RT0Properties);
        scheduler->NewDepthStencil(ResourceNames::GBufferDepthStencil);
        scheduler->WillUseRootConstantBuffer<GBufferCBContent>();
    }  

    void GBufferRenderPass::Render(RenderContext* context) 
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GBuffer);
        context->GetCommandRecorder()->SetRenderTargetAndDepthStencil(ResourceNames::GBufferRT0, ResourceNames::GBufferDepthStencil);
        context->GetCommandRecorder()->ClearBackBuffer(Foundation::Color::Black());
        context->GetCommandRecorder()->ClearDepth(ResourceNames::GBufferDepthStencil, 1.0f);

        // Use vertex and index buffers as normal structured buffers
        context->GetCommandRecorder()->BindExternalBuffer(*context->GetVertexStorage()->UnifiedVertexBuffer_1P1N1UV1T1BT(), 0, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*context->GetVertexStorage()->UnifiedIndexBuffer_1P1N1UV1T1BT(), 1, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(context->GetAssetStorage()->InstanceTable(), 2, 0, HAL::ShaderRegister::ShaderResource);

        GBufferCBContent* cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<GBufferCBContent>();
        cbContent->ParallaxCounterTextureUAVIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex("POMCounter");

        context->GetScene()->IterateMeshInstances([&](const MeshInstance& instance)
        {
            cbContent->InstanceTableIndex = instance.GPUInstanceIndex();
            context->GetCommandRecorder()->Draw(instance.AssosiatedMesh()->LocationInVertexStorage().IndexCount);
        });
    }

}
