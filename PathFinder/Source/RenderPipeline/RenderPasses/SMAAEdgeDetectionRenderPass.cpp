#include "SMAAEdgeDetectionRenderPass.hpp"
#include "ResourceNameResolving.hpp"

namespace PathFinder
{

    SMAAEdgeDetectionRenderPass::SMAAEdgeDetectionRenderPass()
        : RenderPass("SMAAEdgeDetection") {}

    void SMAAEdgeDetectionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
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
      
    void SMAAEdgeDetectionRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    { 
        bool isGIDebugEnabled = scheduler->Content()->GetScene()->GlobalIlluminationManager().GIDebugEnabled;

        scheduler->ReadTexture(SMAAEdgeDetectionPassInputSRName(isGIDebugEnabled));
        scheduler->NewRenderTarget(ResourceNames::SMAADetectedEdges, NewTextureProperties{ HAL::ColorFormat::RG8_Usigned_Norm });
    }  

    void SMAAEdgeDetectionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        bool isGIDebugEnabled = context->GetContent()->GetScene()->GlobalIlluminationManager().GIDebugEnabled;

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SMAAEdgeDetection);

        SMAAEdgeDetectionCBContent cbContent{};
        cbContent.InputTexIdx = context->GetResourceProvider()->GetSRTextureIndex(SMAAEdgeDetectionPassInputSRName(isGIDebugEnabled));

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->SetRenderTarget(ResourceNames::SMAADetectedEdges);
        context->GetCommandRecorder()->Draw(DrawablePrimitive::Triangle());
    }

}
