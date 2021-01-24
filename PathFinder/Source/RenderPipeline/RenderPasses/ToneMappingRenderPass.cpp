#include "ToneMappingRenderPass.hpp"
#include "UAVClearHelper.hpp"
#include "ResourceNameResolving.hpp"

#include <Foundation/Gaussian.hpp>

namespace PathFinder
{

    ToneMappingRenderPass::ToneMappingRenderPass()
        : RenderPass("ToneMapping") {}

    void ToneMappingRenderPass::SetupRootSignatures(RootSignatureCreator* rootSignatureCreator)
    {
        rootSignatureCreator->CreateRootSignature(RootSignatureNames::ToneMapping, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddUnorderedAccessBufferParameter(0, 0); // Histogram | u0 - s0
        });
    }

    void ToneMappingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::ToneMapping, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "ToneMapping.hlsl";
            state.RootSignatureName = RootSignatureNames::ToneMapping;
        });
    }

    void ToneMappingRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    {
        bool isGIDebugEnabled = scheduler->Content()->GetScene()->GlobalIlluminationManager().GIDebugEnabled;

        scheduler->ReadTexture(ToneMappingPassInputSRName(isGIDebugEnabled));
        scheduler->NewTexture(ResourceNames::ToneMappingOutput);
        scheduler->NewBuffer(ResourceNames::LuminanceHistogram, NewBufferProperties<uint32_t>{128});
        scheduler->Export(ResourceNames::LuminanceHistogram);
    }
     
    void ToneMappingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        ClearUAVBufferUInt(context, ResourceNames::LuminanceHistogram, 0);

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::ToneMapping);

        const DisplaySettingsController* dsc = context->GetContent()->DisplayController();

        ToneMappingCBContent cbContent{};
        cbContent.InputTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::BloomCompositionOutput);
        cbContent.OutputTexIdx = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::ToneMappingOutput);
        cbContent.TonemappingParams = context->GetContent()->GetScene()->TonemappingParams();
        cbContent.IsHDREnabled = dsc->IsHDREnabled();
        cbContent.DisplayMaximumLuminance = dsc->PrimaryDisplay()->MaxLuminance();

        context->GetCommandRecorder()->BindBuffer(ResourceNames::LuminanceHistogram, 0, 0, HAL::ShaderRegister::UnorderedAccess);
        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(16, 16);
        context->GetCommandRecorder()->Dispatch(dimensions.x, dimensions.y);
    }

}
