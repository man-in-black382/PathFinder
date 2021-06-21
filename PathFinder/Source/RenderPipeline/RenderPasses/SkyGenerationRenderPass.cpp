#include "SkyGenerationRenderPass.hpp"
#include "ResourceNameResolving.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    SkyGenerationRenderPass::SkyGenerationRenderPass()
        : RenderPass("SkyGeneration") {}

    void SkyGenerationRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::SkyGeneration, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "HosekSky.hlsl";
        });
    }

    void SkyGenerationRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        NewTextureProperties skyProperties{ HAL::ColorFormat::RGBA16_Float, HAL::TextureKind::Texture2D, Geometry::Dimensions{1024, 1024} };
        scheduler->NewTexture(ResourceNames::SkyLuminance, skyProperties);
        scheduler->ExecuteOnQueue(RenderPassExecutionQueue::AsyncCompute);
    }
     
    void SkyGenerationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SkyGeneration);

        const RenderSettings* settings = context->GetContent()->GetSettings();
        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();

        const Geometry::Dimensions& skyTexSize = context->GetResourceProvider()->GetTextureProperties(ResourceNames::SkyLuminance).Dimensions;
        auto groupCount = CommandRecorder::DispatchGroupCount(skyTexSize, { 8, 8 });

        auto skyGPURepresentation = sceneStorage->GetSkyGPURepresentation();

        SkyGenerationCBContent cbContent{};
        cbContent.SunDirection = context->GetContent()->GetScene()->GetSky().GetSunDirection();
        cbContent.SkyStateR = skyGPURepresentation[0];
        cbContent.SkyStateG = skyGPURepresentation[1];
        cbContent.SkyStateB = skyGPURepresentation[2];
        cbContent.SkyTexSize = { skyTexSize.Width, skyTexSize.Height };
        cbContent.DispatchGroupCount = { groupCount.Width, groupCount.Height };
        cbContent.SkyTexIdx = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::SkyLuminance);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(groupCount.Width, groupCount.Height);
    }

}
