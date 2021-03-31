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
            HAL::TextureKind::Texture2D,
            Geometry::Dimensions{scheduler->Content()->GetSettings()->GlobalIlluminationSettings.GetTotalProbeCount(), 1}
        };

        indirectionTableProperties.Flags = ResourceSchedulingFlags::CrossFrameRead;

        // Indirection table to track probe "world" indices so that when probe grid is moved,
        // probes that are not just spawned or despawned reference correct atlas positions
        scheduler->NewTexture(ResourceNames::GIIndirectionTable, indirectionTableProperties);
        scheduler->ExecuteOnQueue(RenderPassExecutionQueue::AsyncCompute);
    } 

    void GIProbeIndirectionTableUpdateRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIProbeIndirectionTableUpdate);

        auto resourceProvider = context->GetResourceProvider();

        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();

        GIProbeIndirectionTableUpdateCBContent cbContent{};
        cbContent.ProbeField = sceneStorage->IrradianceFieldGPURepresentation();
        cbContent.ProbeField.IndirectionTableTexIdx = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::GIIndirectionTable);
        cbContent.ShouldInitialize = !mIsTableInitialized;

        mIsTableInitialized = true;
       
        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch({ cbContent.ProbeField.TotalProbeCount }, { 64, 1 });
    }

}
