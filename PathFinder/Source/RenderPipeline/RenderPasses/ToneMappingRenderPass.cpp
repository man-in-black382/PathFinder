#include "ToneMappingRenderPass.hpp"

#include "../Foundation/GaussianFunction.hpp"

namespace PathFinder
{

    ToneMappingRenderPass::ToneMappingRenderPass()
        : RenderPass("ToneMapping") {}

    void ToneMappingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::ToneMapping, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"ToneMappingRenderPass.hlsl";
        });
    }

    void ToneMappingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->WillUseRootConstantBuffer<ToneMappingCBContent>();
        scheduler->ReadTexture(ResourceNames::DeferredLightingOutput);
        scheduler->NewTexture(ResourceNames::ToneMappingOutput);
    }
     
    void ToneMappingRenderPass::Render(RenderContext* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::ToneMapping);

        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<ToneMappingCBContent>();
        cbContent->InputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::DeferredLightingOutput);
        cbContent->OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::ToneMappingOutput);

        auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(32, 32);
        context->GetCommandRecorder()->Dispatch(dimensions.x, dimensions.y);
    }

}
