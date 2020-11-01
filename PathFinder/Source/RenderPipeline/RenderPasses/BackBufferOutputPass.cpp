#include "BackBufferOutputPass.hpp"

namespace PathFinder
{

    BackBufferOutputPass::BackBufferOutputPass()
        : RenderPass("BackBufferOutput") {}

    void BackBufferOutputPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        bool isHDR = false;

        auto configurator = [&isHDR](GraphicsStateProxy& state)
        {
            state.VertexShaderFileName = "BackBufferOutput.hlsl";
            state.PixelShaderFileName = "BackBufferOutput.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleStrip;
            state.DepthStencilState.SetDepthTestEnabled(false);
            state.RenderTargetFormats = { 
                isHDR ? 
                HAL::SwapChain::HDRBackBufferFormat : 
                HAL::SwapChain::SDRBackBufferFormat 
            };
        };

        stateCreator->CreateGraphicsState(PSONames::SDRBackBufferOutput, configurator);

        isHDR = true;
        stateCreator->CreateGraphicsState(PSONames::HDRBackBufferOutput, configurator);
    }
     
    void BackBufferOutputPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->ReadTexture(ResourceNames::UIOutput);
        scheduler->WriteToBackBuffer();
    } 

    void BackBufferOutputPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        Foundation::Name psoName = context->GetContent()->DisplayController()->IsHDREnabled() ? 
            PSONames::HDRBackBufferOutput : PSONames::SDRBackBufferOutput;
        
        context->GetCommandRecorder()->ApplyPipelineState(psoName);
        context->GetCommandRecorder()->SetBackBufferAsRenderTarget();
    
        BackBufferOutputPassData cbContent;
        cbContent.SourceTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::UIOutput);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Draw(DrawablePrimitive::Quad());
    }

}
