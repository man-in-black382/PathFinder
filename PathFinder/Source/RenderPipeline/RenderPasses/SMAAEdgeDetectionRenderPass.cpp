#include "SMAAEdgeDetectionRenderPass.hpp"

namespace PathFinder
{

    SMAAEdgeDetectionRenderPass::SMAAEdgeDetectionRenderPass()
        : RenderPass("SMAAEdgeDetection") {}

    void SMAAEdgeDetectionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateGraphicsState(PSONames::SMAAEdgeDetection, [](GraphicsStateProxy& state)
        {
            state.VertexShaderFileName = "SMAAEdgeDetection.hlsl";
            state.PixelShaderFileName = "SMAAEdgeDetection.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.DepthStencilState.SetDepthTestEnabled(false);
            state.RenderTargetFormats = { HAL::ColorFormat::RG8_Usigned_Norm };
        });
    }
      
    void SMAAEdgeDetectionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->ReadTexture(ResourceNames::ToneMappingOutput);
        scheduler->NewRenderTarget(ResourceNames::SMAADetectedEdges, ResourceScheduler::NewTextureProperties{ HAL::ColorFormat::RG8_Usigned_Norm });
    }  

    void SMAAEdgeDetectionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SMAAEdgeDetection);

        SMAAEdgeDetectionCBContent cbContent{};
        cbContent.InputTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::ToneMappingOutput);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->SetRenderTarget(ResourceNames::SMAADetectedEdges);
        context->GetCommandRecorder()->Draw(DrawablePrimitive::Triangle());
    }

}
