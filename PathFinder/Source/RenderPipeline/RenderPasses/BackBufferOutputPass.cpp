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
        scheduler->WillUseRootConstantBuffer<BackBufferOutputPassData>();

    /*    scheduler->NewTexture("5", ResourceScheduler::NewTextureProperties{ std::nullopt,  Geometry::Dimensions{500, 100}, std::nullopt, std::nullopt });
        scheduler->NewTexture("6", ResourceScheduler::NewTextureProperties{ std::nullopt,  Geometry::Dimensions{600, 100}, std::nullopt, std::nullopt });*/
    } 

    void BackBufferOutputPass::Render(RenderContext* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BackBufferOutput);
        context->GetCommandRecorder()->SetBackBufferAsRenderTarget();
    
        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<BackBufferOutputPassData>();
        cbContent->SourceTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::BlurResult);

        context->GetCommandRecorder()->Draw(DrawablePrimitive::Quad());
    }

}
