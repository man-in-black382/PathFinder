#include "DenoiserGradientFilteringRenderPass.hpp"
#include "UAVClearHelper.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    DenoiserGradientFilteringRenderPass::DenoiserGradientFilteringRenderPass()
        : RenderPass("DenoiserGradientFiltering") {}

    void DenoiserGradientFilteringRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserGradientFiltering, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserGradientAtrousWaveletFilter.hlsl";
        });
    }

    void DenoiserGradientFilteringRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->AliasAndWriteTexture(ResourceNames::DenoiserPrimaryGradient, ResourceNames::DenoiserPrimaryGradientFilteredIntermediate);
        scheduler->NewTexture(ResourceNames::DenoiserPrimaryGradientFiltered, ResourceScheduler::NewTextureProperties{ ResourceNames::DenoiserPrimaryGradientInputs });
    }
     
    void DenoiserGradientFilteringRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserGradientFiltering);

        auto resourceProvider = context->GetResourceProvider();

        const Geometry::Dimensions& outputDimensions = resourceProvider->GetTextureProperties(ResourceNames::DenoiserPrimaryGradientFiltered).Dimensions;
        auto inputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoiserPrimaryGradientFilteredIntermediate);
        auto outputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DenoiserPrimaryGradientFiltered);

        DenoiserGradientFilteringCBContent cbContent{};
        cbContent.ImageSize = { outputDimensions.Width, outputDimensions.Height };

        for (auto i = 0u; i < 3u; ++i)
        {
            cbContent.CurrentIteration = i;
            cbContent.InputTexIdx = inputTexIdx;
            cbContent.OutputTexIdx = outputTexIdx;

            context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
            context->GetCommandRecorder()->Dispatch(outputDimensions, { 16, 16 });

            std::swap(inputTexIdx, outputTexIdx);
        }
    }

}
