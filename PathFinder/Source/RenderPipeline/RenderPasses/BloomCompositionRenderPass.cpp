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
        scheduler->ReadTexture(ResourceNames::CombinedShading);
        scheduler->ReadTexture(ResourceNames::BloomBlurOutput);
        scheduler->NewTexture(ResourceNames::BloomCompositionOutput);
    }
     
    void BloomCompositionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::BloomComposition);

        auto resourceProvider = context->GetResourceProvider();
        auto dimensions = context->GetDefaultRenderSurfaceDesc().Dimensions();

        const BloomParameters& parameters = context->GetContent()->GetScene()->BloomParams();

        BloomCompositionCBContent inputs{};
        inputs.InverseTextureDimensions = { 1.0f / dimensions.Width, 1.0f / dimensions.Height };
        inputs.CombinedShadingTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::CombinedShading);
        inputs.BloomBlurOutputTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::BloomBlurOutput);
        inputs.CompositionOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::BloomCompositionOutput);
        inputs.SmallBloomWeight = parameters.SmallBloomWeight;
        inputs.MediumBloomWeight = parameters.MediumBloomWeight;
        inputs.LargeBloomWeight = parameters.LargeBloomWeight;

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(inputs);
        context->GetCommandRecorder()->Dispatch(dimensions, { 32, 32 });
    }

}
