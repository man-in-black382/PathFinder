#include "BloomCompositionRenderPass.hpp"
#include "UAVClearHelper.hpp"

namespace PathFinder
{

    BloomCompositionRenderPass::BloomCompositionRenderPass()
        : RenderPass("BloomComposition") {}

    void BloomCompositionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        rootSignatureCreator->CreateRootSignature(RootSignatureNames::BloomComposition, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddUnorderedAccessBufferParameter(0, 0); // Histogram | u0 - s0
        });

        stateCreator->CreateComputeState(PSONames::BloomComposition, [this](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "BloomComposition.hlsl";
            state.RootSignatureName = RootSignatureNames::BloomComposition;
        });
    }

    void BloomCompositionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->ReadTexture(ResourceNames::CombinedShading);
        scheduler->ReadTexture(ResourceNames::BloomBlurOutput);

        // Read min/max luminances
        scheduler->ReadTexture(ResourceNames::CombinedShading, ResourceScheduler::MipSet::LastMip());

        scheduler->NewTexture(ResourceNames::BloomCompositionOutput);

        scheduler->NewBuffer(ResourceNames::LuminanceHistogram, ResourceScheduler::NewBufferProperties<uint32_t>{130}); // 128 + 2 slots for min/max luminance
        scheduler->Export(ResourceNames::LuminanceHistogram);
    }
     
    void BloomCompositionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        ClearUAVBufferUInt(context, ResourceNames::LuminanceHistogram, 0);

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
        inputs.CombinedShadingLastMipIdx = resourceProvider->GetTextureProperties(ResourceNames::CombinedShading).MipCount - 1;

        context->GetCommandRecorder()->BindBuffer(ResourceNames::LuminanceHistogram, 0, 0, HAL::ShaderRegister::UnorderedAccess);
        context->GetConstantsUpdater()->UpdateRootConstantBuffer(inputs);
        context->GetCommandRecorder()->Dispatch(dimensions, { 16, 16 });
    }

}
