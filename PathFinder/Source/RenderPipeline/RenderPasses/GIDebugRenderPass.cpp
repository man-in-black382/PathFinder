#include "GIDebugRenderPass.hpp"

namespace PathFinder
{

    GIDebugRenderPass::GIDebugRenderPass()
        : RenderPass("GIDebug") {} 

    void GIDebugRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
         stateCreator->CreateGraphicsState(PSONames::GIProbeDebug, [](GraphicsStateProxy& state)
         {
             state.VertexShaderFileName = "GIDebug.hlsl";
             state.PixelShaderFileName = "GIDebug.hlsl";
             state.VertexShaderEntryPoint = "ProbeVSMain";
             state.PixelShaderEntryPoint = "ProbePSMain";
             state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
             state.DepthStencilState.SetDepthTestEnabled(true);
             state.RenderTargetFormats = { HAL::ColorFormat::RGBA16_Float };
         });

         stateCreator->CreateGraphicsState(PSONames::GIRaysDebug, [](GraphicsStateProxy& state)
         {
             state.VertexShaderFileName = "GIDebug.hlsl";
             state.PixelShaderFileName = "GIDebug.hlsl";
             state.VertexShaderEntryPoint = "RaysVSMain";
             state.PixelShaderEntryPoint = "RaysPSMain";
             state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
             state.DepthStencilState.SetDepthTestEnabled(true);
             state.RenderTargetFormats = { HAL::ColorFormat::RGBA16_Float };
         });  
    }
     
    void GIDebugRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    { 
        if (!scheduler->Content()->GetSettings()->IsGIDebugEnabled)
            return;

        scheduler->ReadTexture(ResourceNames::GIRayHitInfo);
        scheduler->ReadTexture(ResourceNames::GIIrradianceProbeAtlas);
        scheduler->ReadTexture(ResourceNames::GIDepthProbeAtlas);
        scheduler->ReadTexture(ResourceNames::GIIndirectionTable);
        scheduler->AliasAndUseRenderTarget(ResourceNames::BloomCompositionOutput, ResourceNames::GIDebugOutput);
        scheduler->AliasAndUseDepthStencil(ResourceNames::GBufferDepthStencil, ResourceNames::GIDebugDepthStencil);
    } 

    void GIDebugRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIProbeDebug);

        auto resourceProvider = context->GetResourceProvider();

        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();
        const GIManager& giManager = context->GetContent()->GetScene()->GlobalIlluminationManager();

        GIDebugCBContent cbContent{};
        cbContent.ProbeField = sceneStorage->IrradianceFieldGPURepresentation();
        cbContent.ProbeField.RayHitInfoTextureIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIRayHitInfo);
        cbContent.ProbeField.IrradianceProbeAtlasTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIIrradianceProbeAtlas);
        cbContent.ProbeField.DepthProbeAtlasTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIDepthProbeAtlas);
        cbContent.ProbeField.IndirectionTableTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIIndirectionTable);
        cbContent.ExplicitProbeIndex = giManager.PickedDebugProbeIndex.value_or(-1);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        // Draw on top of existing RT and Depth buffer that were formed in GBuffer pass
        context->GetCommandRecorder()->SetRenderTarget(ResourceNames::GIDebugOutput, ResourceNames::GIDebugDepthStencil);
        
        // 6 for 2 triangles representing a quad
        // Draw 1 specific probe if it was picked by the user
        auto vertexCount = giManager.PickedDebugProbeIndex ?
            DrawablePrimitive::UnitQuadIndexCount :
            cbContent.ProbeField.TotalProbeCount * DrawablePrimitive::UnitQuadIndexCount;

        context->GetCommandRecorder()->Draw(vertexCount);

        if (giManager.PickedDebugProbeIndex)
        {
            // Draw probe rays if a particular probe is selected
            context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIRaysDebug);
            context->GetCommandRecorder()->Draw(DrawablePrimitive::UnitCubeIndexCount * cbContent.ProbeField.RaysPerProbe);
        }
    }

}
