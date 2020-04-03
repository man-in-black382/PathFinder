#include "ToneMappingRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    ToneMappingRenderPass::ToneMappingRenderPass()
        : RenderPass("ToneMapping") {}

    void ToneMappingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::ToneMapping, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "ToneMappingRenderPass.hlsl";
        });
    }

    void ToneMappingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->ReadTexture(ResourceNames::BloomCompositionOutput);
        scheduler->NewTexture(ResourceNames::ToneMappingOutput);


        scheduler->ReadTexture(ResourceNames::AnalyticalLuminanceOutput);
    }
     
    void ToneMappingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::ToneMapping);

        ToneMappingCBContent cbContent{};
        cbContent.InputTextureIndex = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::AnalyticalLuminanceOutput/*BloomCompositionOutput*/);
        cbContent.OutputTextureIndex = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::ToneMappingOutput);
        cbContent.TonemappingParams = context->GetContent()->GetScene()->TonemappingParams();

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(32, 32);
        context->GetCommandRecorder()->Dispatch(dimensions.x, dimensions.y);
    }

}
