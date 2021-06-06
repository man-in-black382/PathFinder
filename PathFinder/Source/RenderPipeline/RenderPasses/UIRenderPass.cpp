#include "UIRenderPass.hpp"

#include "../CommonBlendStates.hpp"

namespace PathFinder
{

    UIRenderPass::UIRenderPass()
        : RenderPass("UI") {}

    void UIRenderPass::SetupRootSignatures(RootSignatureCreator* rootSignatureCreator)
    {
        rootSignatureCreator->CreateRootSignature(RootSignatureNames::UI, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddRootConstantsParameter<UIRootConstants>(0, 0); 
            signatureProxy.AddShaderResourceBufferParameter(0, 0); // UI Vertex Buffer
            signatureProxy.AddShaderResourceBufferParameter(1, 0); // UI Index Buffer
        });
    }

    void UIRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateGraphicsState(PSONames::UI, [](GraphicsStateProxy& state)
        {
            state.VertexShaderFileName = "UIRenderPass.hlsl";
            state.PixelShaderFileName = "UIRenderPass.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.RootSignatureName = RootSignatureNames::UI;
            state.DepthStencilState.SetDepthTestEnabled(false);
            state.RasterizerState.SetFrontClockwise(true); // ImGui is Clockwise for front face
            state.RasterizerState.SetCullMode(HAL::RasterizerState::CullMode::None);
            state.BlendState = AlphaBlendingState();
        });
    }
      
    void UIRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    { 
        scheduler->AliasAndUseRenderTarget(ResourceNames::ToneMappingOutput, ResourceNames::UIOutput);
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
            UIRootConstants constants;
            constants.VertexBufferOffset = drawCommand.VertexBufferOffset;
            constants.IndexBufferOffset = drawCommand.IndexBufferOffset;
            
            if (const UITextureData* textureData = drawCommand.TextureData)
            {
                constants.TextureIdx = textureData->Texture->GetSRDescriptor()->IndexInHeapRange();
                switch (textureData->SamplerMode)
                {
                case UITextureData::SamplingMode::Linear: 
                    constants.SamplerIdx = context->GetResourceProvider()->GetSamplerIndex(SamplerNames::LinearClamp);
                    break;
                case UITextureData::SamplingMode::Point:
                default:
                    constants.SamplerIdx = context->GetResourceProvider()->GetSamplerIndex(SamplerNames::PointClamp);
                    break;
                }
            }
            else
            {
                constants.TextureIdx = -1;
                constants.SamplerIdx = 0;
            }
             
            context->GetCommandRecorder()->SetRootConstants(constants, 0, 0);
            context->GetCommandRecorder()->SetScissor(drawCommand.ScissorRect);
            context->GetCommandRecorder()->Draw(drawCommand.IndexCount);
        }
    }

}
