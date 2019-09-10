#include "BackBufferOutputPass.hpp"

namespace PathFinder
{

    BackBufferOutputPass::BackBufferOutputPass()
        : RenderPass("BackBufferOutput") {}

    void BackBufferOutputPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
       /* auto pso = psoManager->CloneDefaultGraphicsState(); 
        pso.SetShaders(shaderManager->LoadShaders("BackBufferOutput.hlsl", "BackBufferOutput.hlsl")); 
        pso.SetRenderTargetFormats(HAL::ResourceFormat::Color::RGBA8_Usigned_Norm);
        pso.SetPrimitiveTopology(HAL::PrimitiveTopology::TriangleStrip); 
        pso.GetDepthStencilState().SetDepthTestEnabled(false);
        psoManager->StoreGraphicsState(PSONames::BackBufferOutput, pso, RootSignatureNames::Universal);   */
    }
     
    void BackBufferOutputPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->ReadTexture(ResourceNames::BlurResult);   
        scheduler->WillUseRootConstantBuffer<BackBufferOutputPassData>();
    } 

    void BackBufferOutputPass::Render(RenderContext* context)
    {
        context->GetGraphicsDevice()->ApplyPipelineState(PSONames::BackBufferOutput);
        context->GetGraphicsDevice()->SetBackBufferAsRenderTarget();
        context->GetGraphicsDevice()->SetViewport({ 1280, 720 });
    
        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<BackBufferOutputPassData>();
        cbContent->SourceTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::BlurResult);

        context->GetGraphicsDevice()->Draw(DrawablePrimitive::Quad());
    }

}
