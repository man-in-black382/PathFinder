#include "UIRenderPass.hpp"

namespace PathFinder
{

    UIRenderPass::UIRenderPass()
        : RenderPass("UI") {}

    void UIRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature UISinature = stateCreator->CloneBaseRootSignature();
        UISinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 }); // UI Vertex Buffer
        UISinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 1, 0 }); // UI Index Buffer
        stateCreator->StoreRootSignature(RootSignatureNames::UI, std::move(UISinature));

        stateCreator->CreateGraphicsState(PSONames::UI, [](GraphicsStateProxy& state)
        {
            state.ShaderFileNames.VertexShaderFileName = L"UIRenderPass.hlsl";
            state.ShaderFileNames.PixelShaderFileName = L"UIRenderPass.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::UI;
            state.DepthStencilState.SetDepthTestEnabled(false);
        });
    }
      
    void UIRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->WillUseRootConstantBuffer<UICBContent>();
    }  

    void UIRenderPass::Render(RenderContext* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::UI);
        context->GetCommandRecorder()->SetBackBufferAsRenderTarget();

        context->GetCommandRecorder()->BindExternalBuffer(*context->GetUIStorage()->VertexBuffer(), 0, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*context->GetUIStorage()->IndexBuffer(), 1, 0, HAL::ShaderRegister::ShaderResource);
    }

}
