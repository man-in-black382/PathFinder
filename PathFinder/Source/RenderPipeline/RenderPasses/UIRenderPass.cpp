#include "UIRenderPass.hpp"

#include "../CommonBlendStates.hpp"

namespace PathFinder
{

    UIRenderPass::UIRenderPass()
        : RenderPass("UI") {}

    void UIRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        rootSignatureCreator->CreateRootSignature(RootSignatureNames::UI, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddRootConstantsParameter<UIRootConstants>(0, 0); // Vertex/Index offsets. 2 root constants
            signatureProxy.AddShaderResourceBufferParameter(0, 0); // UI Vertex Buffer
            signatureProxy.AddShaderResourceBufferParameter(1, 0); // UI Index Buffer
        });

        stateCreator->CreateGraphicsState(PSONames::UI, [](GraphicsStateProxy& state)
        {
            state.VertexShaderFileName = "UIRenderPass.hlsl";
            state.PixelShaderFileName = "UIRenderPass.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::UI;
            state.DepthStencilState.SetDepthTestEnabled(false);
            state.RasterizerState.SetFrontClockwise(true); // ImGui is Clockwise for front face
            state.BlendState = AlphaBlendingState();
        });
    }
      
    void UIRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->AliasAndUseRenderTarget(ResourceNames::SMAAAntialiased, ResourceNames::UIOutput);
    }  

    void UIRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::UI);
        context->GetCommandRecorder()->SetRenderTarget(ResourceNames::UIOutput);

        if (auto vertexBuffer = context->GetContent()->GetUIGPUStorage()->VertexBuffer())
        {
            context->GetCommandRecorder()->BindExternalBuffer(*vertexBuffer, 0, 0, HAL::ShaderRegister::ShaderResource);
        }

        if (auto indexBuffer = context->GetContent()->GetUIGPUStorage()->IndexBuffer())
        {
            context->GetCommandRecorder()->BindExternalBuffer(*indexBuffer, 1, 0, HAL::ShaderRegister::ShaderResource);
        }

        UICBContent cbContent{};
        cbContent.ProjectionMatrix = context->GetContent()->GetUIGPUStorage()->MVP();
        cbContent.UITextureSRVIndex = context->GetContent()->GetUIGPUStorage()->FontTexture()->GetSRDescriptor()->IndexInHeapRange();

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        for (const UIGPUStorage::DrawCommand& drawCommand : context->GetContent()->GetUIGPUStorage()->DrawCommands())
        {
            UIRootConstants offsets;
            offsets.VertexBufferOffset = drawCommand.VertexBufferOffset;
            offsets.IndexBufferOffset = drawCommand.IndexBufferOffset;

            context->GetCommandRecorder()->SetRootConstants(offsets, 0, 0);
            context->GetCommandRecorder()->Draw(drawCommand.IndexCount);
        }
    }

}
