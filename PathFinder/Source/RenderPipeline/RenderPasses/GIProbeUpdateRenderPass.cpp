#include "GIProbeUpdateRenderPass.hpp"

#include <Foundation/Halton.hpp>

namespace PathFinder
{

    GIProbeUpdateRenderPass::GIProbeUpdateRenderPass()
        : RenderPass("GIProbeUpdate") {} 

    void GIProbeUpdateRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::GIProbeUpdate, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "GIProbeUpdate.hlsl";
        });

        stateCreator->CreateComputeState(PSONames::GIIlluminanceProbeCornerUpdate, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "GIProbeBorderUpdate.hlsl";
            state.EntryPoint = "IlluminanceCornerUpdateCSMain";
        });

        stateCreator->CreateComputeState(PSONames::GIDepthProbeCornerUpdate, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "GIProbeBorderUpdate.hlsl";
            state.EntryPoint = "DepthCornerUpdateCSMain";
        });

        stateCreator->CreateComputeState(PSONames::GIIlluminanceProbeBorderUpdate, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "GIProbeBorderUpdate.hlsl";
            state.EntryPoint = "IlluminanceBorderUpdateCSMain";
        });

        stateCreator->CreateComputeState(PSONames::GIDepthProbeBorderUpdate, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "GIProbeBorderUpdate.hlsl";
            state.EntryPoint = "DepthBorderUpdateCSMain";
        });
    }
     
    void GIProbeUpdateRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    { 
        auto previousFrameIdx = (scheduler->GetFrameNumber() - 1) % 2;
        auto currentFrameIdx = (scheduler->GetFrameNumber()) % 2;

        NewTextureProperties irradianceTextureProperties{
            HAL::ColorFormat::RGBA16_Float,
            HAL::TextureKind::Texture2D,
            scheduler->GetContent()->GetSettings()->GlobalIlluminationSettings.GetIlluminanceProbeAtlasSize()
        };

        NewTextureProperties depthTextureProperties{
            HAL::ColorFormat::RG16_Float,
            HAL::TextureKind::Texture2D,
            scheduler->GetContent()->GetSettings()->GlobalIlluminationSettings.GetDepthProbeAtlasSize()
        };

        irradianceTextureProperties.Flags = ResourceSchedulingFlags::CrossFrameRead;
        depthTextureProperties.Flags = ResourceSchedulingFlags::CrossFrameRead;

        scheduler->NewTexture(ResourceNames::GIIlluminanceProbeAtlas[currentFrameIdx], irradianceTextureProperties);
        scheduler->NewTexture(ResourceNames::GIDepthProbeAtlas[currentFrameIdx], depthTextureProperties);

        // Indicate that these textures are created, but not written
        scheduler->NewTexture(ResourceNames::GIIlluminanceProbeAtlas[previousFrameIdx], MipSet::Empty(), irradianceTextureProperties);
        scheduler->NewTexture(ResourceNames::GIDepthProbeAtlas[previousFrameIdx], MipSet::Empty(), depthTextureProperties);

        // Read previous frame atlases
        scheduler->ReadTexture(ResourceNames::GIIlluminanceProbeAtlas[previousFrameIdx]);
        scheduler->ReadTexture(ResourceNames::GIDepthProbeAtlas[previousFrameIdx]);

        scheduler->ReadTexture(ResourceNames::GIRayHitInfo);

        scheduler->ExecuteOnQueue(RenderPassExecutionQueue::AsyncCompute);
    } 

    void GIProbeUpdateRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIProbeUpdate);

        auto previousFrameIdx = (context->GetFrameNumber() - 1) % 2;
        auto currentFrameIdx = (context->GetFrameNumber()) % 2;
        auto resourceProvider = context->GetResourceProvider();

        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();

        GIProbeUpdateCBContent cbContent{};
        cbContent.ProbeField = sceneStorage->GetIlluminanceFieldGPURepresentation();
        cbContent.ProbeField.RayHitInfoTextureIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIRayHitInfo);
        cbContent.ProbeField.PreviousIlluminanceProbeAtlasTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIIlluminanceProbeAtlas[previousFrameIdx]);
        cbContent.ProbeField.PreviousDepthProbeAtlasTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GIDepthProbeAtlas[previousFrameIdx]);
        cbContent.ProbeField.CurrentIlluminanceProbeAtlasTexIdx = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::GIIlluminanceProbeAtlas[currentFrameIdx]);
        cbContent.ProbeField.CurrentDepthProbeAtlasTexIdx = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::GIDepthProbeAtlas[currentFrameIdx]);
       
        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        // Build atlas
        auto depthProbeTexelCount = cbContent.ProbeField.DepthProbeSize * cbContent.ProbeField.DepthProbeSize;
        context->GetCommandRecorder()->Dispatch({ cbContent.ProbeField.TotalProbeCount * depthProbeTexelCount, 1 }, { depthProbeTexelCount, 1 });

        // Propagate corners 
        const uint64_t CornerCount = 4;

        // Here X dimension represents probes and Y represents 4 corners.
        // One dispatched group handles corners for multiple probes.
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIIlluminanceProbeCornerUpdate);
        context->GetCommandRecorder()->Dispatch({ cbContent.ProbeField.TotalProbeCount, CornerCount }, { 16, CornerCount });

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIDepthProbeCornerUpdate);
        context->GetCommandRecorder()->Dispatch({ cbContent.ProbeField.TotalProbeCount, CornerCount }, { 16, CornerCount });
        
        // Propagate borders 
        // Here each group handles borders for exactly one probe, so we just dispatch ProbeCount groups
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIIlluminanceProbeBorderUpdate);
        context->GetCommandRecorder()->Dispatch(cbContent.ProbeField.TotalProbeCount);

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GIDepthProbeBorderUpdate);
        context->GetCommandRecorder()->Dispatch(cbContent.ProbeField.TotalProbeCount);
    }

}
