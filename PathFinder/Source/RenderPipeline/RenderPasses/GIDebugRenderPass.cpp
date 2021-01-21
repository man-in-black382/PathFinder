#include "GIDebugRenderPass.hpp"

namespace PathFinder
{

    GIDebugRenderPass::GIDebugRenderPass()
        : RenderPass("GIDebug") {} 

    void GIDebugRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        /* stateCreator->CreateGraphicsState(PSONames::GIDebug, [](GraphicsStateProxy& state)
         {
             state.VertexShaderFileName = "GIDebug.hlsl";
             state.PixelShaderFileName = "GIDebug.hlsl";
             state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
             state.DepthStencilState.SetDepthTestEnabled(true);
             state.RenderTargetFormats = { HAL::ColorFormat::RGBA16_Float };
         });*/
    }
     
    void GIDebugRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    { 
        //scheduler->ReadTexture(ResourceNames::GIIrradianceProbeAtlas);
        //scheduler->ReadTexture(ResourceNames::GIDepthProbeAtlas);
        //scheduler->AliasAndUseRenderTarget(ResourceNames::BloomCompositionOutput, ResourceNames::GIDebugOutput);
        //scheduler->AliasAndUseDepthStencil(ResourceNames::GBufferDepthStencil, ResourceNames::GIDebugDepthStencil);
    } 

    void GIDebugRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        //context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIDebug);

        //auto resourceProvider = context->GetResourceProvider();

        //const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();

        //GIDebugCBContent cbContent{};
        //cbContent.ProbeField = sceneStorage->IrradianceFieldGPURepresentation();
        //cbContent.ProbeField.IrradianceProbeAtlasTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIIrradianceProbeAtlas);
        //cbContent.ProbeField.DepthProbeAtlasTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIDepthProbeAtlas);
       
        //context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        //// Draw on top of existing RT and Depth buffer that were formed in GBuffer pass
        //context->GetCommandRecorder()->SetRenderTarget(ResourceNames::GIDebugOutput, ResourceNames::GIDebugDepthStencil);


    }

}
