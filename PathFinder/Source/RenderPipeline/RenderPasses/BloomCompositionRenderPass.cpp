#include "BloomCompositionRenderPass.hpp"

namespace PathFinder
{

    BloomCompositionRenderPass::BloomCompositionRenderPass()
        : RenderPass("BloomComposition") {}

    void BloomCompositionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::BloomComposition, [this](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "BloomComposition.hlsl";
        });
    }

    void BloomCompositionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->ReadTexture(ResourceNames::DeferredLightingFullOutput);
        scheduler->ReadTexture(ResourceNames::BloomBlurOutput);
        scheduler->NewTexture(ResourceNames::BloomCompositionOutput);
    }
     
    void BloomCompositionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BloomComposition);

        auto resourceProvider = context->GetResourceProvider();
        auto dimensions = context->GetDefaultRenderSurfaceDesc().Dimensions();

        BloomCompositionCBContent inputs{};
        inputs.InverseTextureDimensions = { 1.0f / dimensions.Width, 1.0f / dimensions.Height };
        inputs.DeferredLightingOutputTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::DeferredLightingFullOutput);
        inputs.BloomBlurOutputTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::BloomBlurOutput);
        inputs.CompositionOutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::BloomCompositionOutput);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(inputs);
        context->GetCommandRecorder()->Dispatch(dimensions, { 32, 32 });
    }

}
