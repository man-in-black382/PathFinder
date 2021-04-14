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
        bool isGIDebugEnabled = scheduler->Content()->GetSettings()->IsGIDebugEnabled;

        scheduler->ReadTexture(SMAAEdgeDetectionPassInputTexName(isGIDebugEnabled), TextureReadContext::PixelShader);
        scheduler->NewRenderTarget(ResourceNames::SMAADetectedEdges, NewTextureProperties{ HAL::ColorFormat::RG8_Usigned_Norm });
    }  

    void SMAAEdgeDetectionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        bool isGIDebugEnabled = context->GetContent()->GetSettings()->IsGIDebugEnabled;

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SMAAEdgeDetection);

        SMAAEdgeDetectionCBContent cbContent{};
        cbContent.InputTexIdx = context->GetResourceProvider()->GetSRTextureIndex(SMAAEdgeDetectionPassInputTexName(isGIDebugEnabled));

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->SetRenderTarget(ResourceNames::SMAADetectedEdges);
        context->GetCommandRecorder()->Draw(DrawablePrimitive::Triangle());
    }

}
