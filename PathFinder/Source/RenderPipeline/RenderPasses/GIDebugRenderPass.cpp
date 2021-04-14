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

        auto currentFrameIdx = (scheduler->FrameNumber()) % 2;

        scheduler->ReadTexture(ResourceNames::GIRayHitInfo);
        scheduler->ReadTexture(ResourceNames::GIIrradianceProbeAtlas[currentFrameIdx]);
        scheduler->ReadTexture(ResourceNames::GIDepthProbeAtlas[currentFrameIdx]);
        scheduler->AliasAndUseRenderTarget(ResourceNames::BloomCompositionOutput, ResourceNames::GIDebugOutput);
        scheduler->AliasAndUseDepthStencil(ResourceNames::GBufferDepthStencil, ResourceNames::GIDebugDepthStencil);
    } 

    void GIDebugRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIProbeDebug);

        auto currentFrameIdx = (context->FrameNumber()) % 2;
        auto resourceProvider = context->GetResourceProvider();

        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();
        const GIManager& giManager = context->GetContent()->GetScene()->GlobalIlluminationManager();

        GIDebugCBContent cbContent{};
        cbContent.ProbeField = sceneStorage->IrradianceFieldGPURepresentation();
        cbContent.ProbeField.RayHitInfoTextureIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIRayHitInfo);
        cbContent.ProbeField.CurrentIrradianceProbeAtlasTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIIrradianceProbeAtlas[currentFrameIdx]);
        cbContent.ProbeField.CurrentDepthProbeAtlasTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIDepthProbeAtlas[currentFrameIdx]);
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
