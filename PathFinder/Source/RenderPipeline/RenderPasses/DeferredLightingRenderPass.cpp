#include "DeferredLightingRenderPass.hpp"

namespace PathFinder
{

    DeferredLightingRenderPass::DeferredLightingRenderPass()
        : RenderPass("DeferredLighting") {}

    void DeferredLightingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateGraphicsState(PSONames::DeferredLighting, [](GraphicsStateProxy& state)
        {
            state.ShaderFileNames.VertexShaderFileName = L"DeferredLightingRenderPass.hlsl";
            state.ShaderFileNames.PixelShaderFileName = L"DeferredLightingRenderPass.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleStrip;
            state.DepthStencilState.SetDepthTestEnabled(false);
        });
    }
      
    void DeferredLightingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->NewRenderTarget(ResourceNames::DeferredLightingRT);
        scheduler->ReadTexture(ResourceNames::GBufferRT0);
        scheduler->WillUseRootConstantBuffer<DeferredLightingCBContent>();
    }  

    void DeferredLightingRenderPass::Render(RenderContext* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DeferredLighting);
        context->GetCommandRecorder()->SetRenderTarget(ResourceNames::DeferredLightingRT);

        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<DeferredLightingCBContent>();
        cbContent->GBufferMaterialDataTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferRT0);

        context->GetCommandRecorder()->Draw(DrawablePrimitive::Quad());
    }

}
