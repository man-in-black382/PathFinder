#include "DenoiserPostStabilizationRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserPostStabilizationRenderPass::DenoiserPostStabilizationRenderPass()
        : RenderPass("DenoiserPostStabilization") {}

    void DenoiserPostStabilizationRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserPostStabilization, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserPostStabilization.hlsl";
        });
    }

    void DenoiserPostStabilizationRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->NewTexture(ResourceNames::ShadingStochasticShadowedDenoisedStabilized);
        scheduler->NewTexture(ResourceNames::ShadingStochasticUnshadowedDenoisedStabilized);
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingDenoiseTargetArrayIdx });
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingDenoiseTargetArrayIdx });
    }
     
    void DenoiserPostStabilizationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserPostStabilization);

        auto resourceProvider = context->GetResourceProvider();

        DenoiserPostStabilizationCBContent cbContent{};
        cbContent.InputTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingDenoiseTargetArrayIdx });
        cbContent.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticShadowedDenoisedStabilized);
        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });

        cbContent.InputTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingDenoiseTargetArrayIdx });
        cbContent.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticUnshadowedDenoisedStabilized);
        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
