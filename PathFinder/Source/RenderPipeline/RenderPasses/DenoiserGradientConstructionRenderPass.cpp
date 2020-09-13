#include "DenoiserGradientConstructionRenderPass.hpp"
#include "UAVClearHelper.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserGradientConstructionRenderPass::DenoiserGradientConstructionRenderPass()
        : RenderPass("DenoiserGradientConstruction") {}

    void DenoiserGradientConstructionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserGradientConstruction, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserGradientConstruction.hlsl";
        });
    }

    void DenoiserGradientConstructionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto currentFrameIndex = scheduler->FrameNumber() % 2;

        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingPreBlurred);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred);
        scheduler->ReadTexture(ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex]);
        scheduler->ReadTexture(ResourceNames::DenoiserPrimaryGradientInputs);

        scheduler->NewTexture(ResourceNames::DenoiserPrimaryGradient, ResourceScheduler::NewTextureProperties{ ResourceNames::DenoiserPrimaryGradientInputs });
    }
     
    void DenoiserGradientConstructionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserGradientConstruction);

        auto currentFrameIndex = context->FrameNumber() % 2;
        auto resourceProvider = context->GetResourceProvider();

        DenoiserGradientConstructionCBContent cbContent{};
        cbContent.GradientInputsTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserPrimaryGradientInputs);
        cbContent.SamplePositionsTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex]);
        cbContent.ShadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingPreBlurred);
        cbContent.UnshadowedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingPreBlurred);
        cbContent.GradientTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoiserPrimaryGradient);

        const Geometry::Dimensions& outputDimensions = resourceProvider->GetTextureProperties(ResourceNames::DenoiserPrimaryGradient).Dimensions;

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(outputDimensions, { 16, 16 });
    }

}
