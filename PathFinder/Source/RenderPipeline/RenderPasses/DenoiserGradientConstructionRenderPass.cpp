#include "DenoiserGradientConstructionRenderPass.hpp"
#include "UAVClearHelper.hpp"
#include "ResourceNameResolving.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    DenoiserGradientConstructionRenderPass::DenoiserGradientConstructionRenderPass()
        : RenderPass("DenoiserGradientConstruction") {}

    void DenoiserGradientConstructionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserGradientConstruction, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserGradientConstruction.hlsl";
        });
    }

    void DenoiserGradientConstructionRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        if (!scheduler->GetContent()->GetSettings()->IsDenoiserEnabled)
            return;

        auto currentFrameIndex = scheduler->GetFrameNumber() % 2;

        scheduler->ReadTexture(DenoiserGradientConstructionShadowedInputTexName(false, currentFrameIndex));
        scheduler->ReadTexture(ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex]);
        scheduler->ReadTexture(ResourceNames::DenoiserGradientSamples);

        scheduler->NewTexture(ResourceNames::DenoiserGradient, NewTextureProperties{ ResourceNames::DenoiserGradientSamples });
    }
     
    void DenoiserGradientConstructionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserGradientConstruction);

        auto currentFrameIndex = context->GetFrameNumber() % 2;
        auto resourceProvider = context->GetResourceProvider();

        DenoiserGradientConstructionCBContent cbContent{};
        cbContent.GradientInputsTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserGradientSamples);
        cbContent.SamplePositionsTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex]);
        cbContent.ShadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(DenoiserGradientConstructionShadowedInputTexName(false, currentFrameIndex));
        cbContent.GradientTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoiserGradient);

        const Geometry::Dimensions& outputDimensions = resourceProvider->GetTextureProperties(ResourceNames::DenoiserGradient).Dimensions;

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(outputDimensions, { 8, 8 });
    }

}
