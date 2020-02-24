#include "DeferredLightingRenderPass.hpp"

namespace PathFinder
{

    DeferredLightingRenderPass::DeferredLightingRenderPass()
        : RenderPass("DeferredLighting") {}

    void DeferredLightingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::DeferredLighting, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"DeferredLightingRenderPass.hlsl";
        });
    }
      
    void DeferredLightingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->NewTexture(ResourceNames::DeferredLightingOutput);
        scheduler->ReadTexture(ResourceNames::GBufferRT0);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
    }  

    void DeferredLightingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DeferredLighting);

        DeferredLightingCBContent cbContent{}; 
        cbContent.GBufferMaterialDataTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferRT0);
        cbContent.GBufferDepthTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferDepthStencil);
        cbContent.OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::DeferredLightingOutput);
        cbContent.LTC_LUT0_Index = context->GetContent()->GetScene()->LTC_LUT0()->GetOrCreateSRDescriptor()->IndexInHeapRange();
        cbContent.LTC_LUT1_Index = context->GetContent()->GetScene()->LTC_LUT1()->GetOrCreateSRDescriptor()->IndexInHeapRange();

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(32, 32);
        context->GetCommandRecorder()->Dispatch(dimensions.x, dimensions.y);
    }

}
