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
        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingDenoisedStabilized);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingDenoisedStabilized);
        scheduler->ReadWriteTexture(ResourceNames::StochasticShadowedShadingDenoised);
        scheduler->ReadWriteTexture(ResourceNames::StochasticUnshadowedShadingDenoised);
    }
     
    void DenoiserPostStabilizationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserPostStabilization);

        auto resourceProvider = context->GetResourceProvider();

        DenoiserPostStabilizationCBContent cbContent{};
        cbContent.InputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingDenoised);
        cbContent.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingDenoisedStabilized);
        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });

        cbContent.InputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingDenoised);
        cbContent.OutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingDenoisedStabilized);
        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
