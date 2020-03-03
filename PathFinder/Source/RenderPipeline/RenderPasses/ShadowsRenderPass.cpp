#include "ShadowsRenderPass.hpp"

namespace PathFinder
{

    ShadowsRenderPass::ShadowsRenderPass()
        : RenderPass("Shadows") {}

    void ShadowsRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateRayTracingState(PSONames::BackBufferOutput, [](RayTracingStateProxy& state)
        {
            RayTracingShaderFileNames shadowShaderFileNames;
            shadowShaderFileNames.RayGenShaderFileName = L"Shadows.hlsl";
            shadowShaderFileNames.MissShaderFileName = L"Shadows.hlsl";

            state.AddShaders(shadowShaderFileNames, HAL::RayTracingShaderConfig{ sizeof(float), 0 });

            state.GlobalRootSignatureName = RootSignatureNames::RayTracing;
            state.PipelineConfig = HAL::RayTracingPipelineConfig{ 1 };
        });
    }
     
    void ShadowsRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
      /*  scheduler->ReadTexture(ResourceNames::BlurResult);
        scheduler->ReadTexture(ResourceNames::GBufferRenderTarget);*/
    } 

    void ShadowsRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {

    }

}
