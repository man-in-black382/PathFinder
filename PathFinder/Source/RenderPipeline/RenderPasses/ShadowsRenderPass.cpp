#include "ShadowsRenderPass.hpp"

namespace PathFinder
{

    ShadowsRenderPass::ShadowsRenderPass()
        : RenderPass("Shadows") {}

    void ShadowsRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature rayTracingSinature = stateCreator->CloneBaseRootSignature();

        rayTracingSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 }); // Scene BVH | t0 - s0
        rayTracingSinature.AddDescriptorParameter(HAL::RootConstantBufferParameter{ 0, 0 }); // Instance Table Constant Buffer | b0 - s0
        rayTracingSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 1, 0 }); // Unified Vertex Buffer | t1 - s0
        rayTracingSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 2, 0 }); // Unified Index Buffer | t2 - s0

        stateCreator->StoreRootSignature(RootSignatureNames::RayTracing, std::move(rayTracingSinature));

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

    void ShadowsRenderPass::Render(RenderContext* context)
    {

    }

}
