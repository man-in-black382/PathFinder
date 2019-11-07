#include "GBufferRenderPass.hpp"

namespace PathFinder
{

    GBufferRenderPass::GBufferRenderPass()
        : RenderPass("GBuffer") {}

    void GBufferRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature GBufferSinature = stateCreator->CloneBaseRootSignature();
        GBufferSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 });
        stateCreator->StoreRootSignature(RootSignatureNames::GBuffer, std::move(GBufferSinature));

        stateCreator->CreateGraphicsState(PSONames::GBuffer, [](GraphicsStateProxy& state)
        {
            state.ShaderFileNames.VertexShaderFileName = L"GBufferRenderPass.hlsl";
            state.ShaderFileNames.PixelShaderFileName = L"GBufferRenderPass.hlsl";
            state.RenderTargetFormats = { HAL::ResourceFormat::Color::RGBA32_Unsigned };
            state.InputLayout = InputAssemblerLayoutForVertexLayout(VertexLayout::Layout1P1N1UV1T1BT);
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
        scheduler->WillUseRootConstantBuffer<GBufferCBContent>();
    }  

    void GBufferRenderPass::Render(RenderContext* context) 
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GBuffer);
        context->GetCommandRecorder()->SetRenderTargetAndDepthStencil(ResourceNames::GBufferRT0, ResourceNames::GBufferDepthStencil);
        context->GetCommandRecorder()->ClearBackBuffer(Foundation::Color::Gray());
        context->GetCommandRecorder()->ClearDepth(ResourceNames::GBufferDepthStencil, 1.0f);
        context->GetCommandRecorder()->UseVertexBufferOfLayout(VertexLayout::Layout1P1N1UV1T1BT);
        context->GetCommandRecorder()->BindMeshInstanceTableStructuredBuffer(0);

        GBufferCBContent* cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<GBufferCBContent>();

        context->GetScene()->IterateMeshInstances([&](const MeshInstance& instance)
        {
            cbContent->InstanceTableIndex = instance.GPUInstanceIndex();
            context->GetCommandRecorder()->Draw(instance.AssosiatedMesh()->LocationInVertexStorage());
        });
    }

}
