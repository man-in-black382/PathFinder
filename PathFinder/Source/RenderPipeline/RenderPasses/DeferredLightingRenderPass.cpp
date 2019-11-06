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
        scheduler->WillUseRootConstantBuffer<DeferredLightingCBContent>();
    }  

    void DeferredLightingRenderPass::Render(RenderContext* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DeferredLighting);

        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<DeferredLightingCBContent>();
        cbContent->GBufferMaterialDataTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferRT0);
        cbContent->GBufferDepthTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferDepthStencil);
        cbContent->OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::DeferredLightingOutput);

        auto dimensions = context->GetDefaultRenderSurfaceDesc().Dimensions();
        float x = ceilf((float)dimensions.Width / 32);
        float y = ceilf((float)dimensions.Height / 32);

        context->GetCommandRecorder()->Dispatch(x, y, 1);
    }

}
