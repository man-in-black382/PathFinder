#include "GIProbeIndirectionTableUpdateRenderPass.hpp"

#include <Foundation/Halton.hpp>

namespace PathFinder
{

    GIProbeIndirectionTableUpdateRenderPass::GIProbeIndirectionTableUpdateRenderPass()
        : RenderPass("GIProbeIndirectionTableUpdate") {} 

    void GIProbeIndirectionTableUpdateRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::GIProbeIndirectionTableUpdate, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "GIProbeIndirectionTableUpdate.hlsl";
        });
    }
     
    void GIProbeIndirectionTableUpdateRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    { 
        NewTextureProperties indirectionTableProperties{
            HAL::ColorFormat::R16_Unsigned,
            HAL::TextureKind::Texture1D,
            scheduler->Content()->GetSettings()->GlobalIlluminationSettings.GetTotalProbeCount()
        };

        scheduler->NewTexture(ResourceNames::GIIndirectionTable, indirectionTableProperties);
        scheduler->ExecuteOnQueue(RenderPassExecutionQueue::AsyncCompute);
    } 

    void GIProbeIndirectionTableUpdateRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        //context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIProbeUpdate);

        //auto resourceProvider = context->GetResourceProvider();

        //const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();

        //GIProbeUpdateCBContent cbContent{};
        //cbContent.ProbeField = sceneStorage->IrradianceFieldGPURepresentation();
        //cbContent.ProbeField.RayHitInfoTextureIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIRayHitInfo);
        //cbContent.ProbeField.IrradianceProbeAtlasTexIdx = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::GIIrradianceProbeAtlas);
        //cbContent.ProbeField.DepthProbeAtlasTexIdx = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::GIDepthProbeAtlas);
       
        //context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        //// Build atlas
        //auto depthProbeTexelCount = cbContent.ProbeField.DepthProbeSize * cbContent.ProbeField.DepthProbeSize;
        //context->GetCommandRecorder()->Dispatch({ cbContent.ProbeField.TotalProbeCount * depthProbeTexelCount, 1 }, { depthProbeTexelCount, 1 });

        //// Propagate corners 
        //const uint64_t CornerCount = 4;

        //// Here X dimension represents probes and Y represents 4 corners.
        //// One dispatched group handles corners for multiple probes.
        //context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIIrradianceProbeCornerUpdate);
        //context->GetCommandRecorder()->Dispatch({ cbContent.ProbeField.TotalProbeCount, CornerCount }, { 16, CornerCount });

        //context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIDepthProbeCornerUpdate);
        //context->GetCommandRecorder()->Dispatch({ cbContent.ProbeField.TotalProbeCount, CornerCount }, { 16, CornerCount });
        //
        //// Propagate borders 
        //// Here each group handles borders for exactly one probe, so we just dispatch ProbeCount groups
        //context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIIrradianceProbeBorderUpdate);
        //context->GetCommandRecorder()->Dispatch(cbContent.ProbeField.TotalProbeCount);

        //context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIDepthProbeBorderUpdate);
        //context->GetCommandRecorder()->Dispatch(cbContent.ProbeField.TotalProbeCount);
    }

}
