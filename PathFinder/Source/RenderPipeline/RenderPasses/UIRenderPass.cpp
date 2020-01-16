#include "UIRenderPass.hpp"

#include "../CommonBlendStates.hpp"

namespace PathFinder
{

    UIRenderPass::UIRenderPass()
        : RenderPass("UI") {}

    void UIRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature UISinature = stateCreator->CloneBaseRootSignature();
        UISinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 }); // UI Vertex Buffer
        UISinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 1, 0 }); // UI Index Buffer
        UISinature.AddConstantsParameter(HAL::RootConstantsParameter{ 2, 0, 0 }); // Vertex/Index offsets. 2 root constants
        stateCreator->StoreRootSignature(RootSignatureNames::UI, std::move(UISinature));

        stateCreator->CreateGraphicsState(PSONames::UI, [](GraphicsStateProxy& state)
        {
            state.ShaderFileNames.VertexShaderFileName = L"UIRenderPass.hlsl";
            state.ShaderFileNames.PixelShaderFileName = L"UIRenderPass.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::UI;
            state.DepthStencilState.SetDepthTestEnabled(false);
            state.RasterizerState.SetFrontClockwise(true); // ImGui is Clockwise for front face
            state.BlendState = AlphaBlendingState();
            state.RenderTargetFormats = { HAL::ColorFormat::RGBA8_Usigned_Norm };
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

        if (auto vertexBuffer = context->GetUIStorage()->VertexBuffer())
        {
            context->GetCommandRecorder()->BindExternalBuffer(*vertexBuffer, 0, 0, HAL::ShaderRegister::ShaderResource);
        }
        
        if (auto indexBuffer = context->GetUIStorage()->IndexBuffer())
        {
            context->GetCommandRecorder()->BindExternalBuffer(*indexBuffer, 1, 0, HAL::ShaderRegister::ShaderResource);
        }

        UICBContent* cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<UICBContent>();
        cbContent->ProjectionMatrix = context->GetUIStorage()->MVP();
        cbContent->UITextureSRVIndex = context->GetUIStorage()->FontSRVIndex();

        for (const UIGPUStorage::DrawCommand& drawCommand : context->GetUIStorage()->DrawCommands())
        {
            UIRootConstants offsets;
            offsets.VertexBufferOffset = drawCommand.VertexBufferOffset;
            offsets.IndexBufferOffset = drawCommand.IndexBufferOffset;

            context->GetCommandRecorder()->SetRootConstants(offsets, 0, 0);
            context->GetCommandRecorder()->Draw(drawCommand.IndexCount);
        }
    }

}
