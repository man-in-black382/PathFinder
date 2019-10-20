#include "BackBufferOutputPass.hpp"

namespace PathFinder
{

    BackBufferOutputPass::BackBufferOutputPass()
        : RenderPass("BackBufferOutput") {}

    void BackBufferOutputPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateGraphicsState(PSONames::BackBufferOutput, [](GraphicsStateProxy& state)
        {
            state.ShaderFileNames.VertexShaderFileName = L"BackBufferOutput.hlsl";
            state.ShaderFileNames.PixelShaderFileName = L"BackBufferOutput.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleStrip;
            state.DepthStencilState.SetDepthTestEnabled(false);
        });
    }
     
    void BackBufferOutputPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->ReadTexture(ResourceNames::BlurResult);
        scheduler->ReadTexture(ResourceNames::GBufferRenderTarget);
        scheduler->WillUseRootConstantBuffer<BackBufferOutputPassData>();
    } 

    void BackBufferOutputPass::Render(RenderContext* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BackBufferOutput);
        context->GetCommandRecorder()->SetBackBufferAsRenderTarget();
    
        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<BackBufferOutputPassData>();
        cbContent->SourceTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferRenderTarget);

        context->GetCommandRecorder()->Draw(DrawablePrimitive::Quad());
    }

}
