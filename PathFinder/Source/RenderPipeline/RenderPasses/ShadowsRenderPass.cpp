#include "ShadowsRenderPass.hpp"

namespace PathFinder
{

    ShadowsRenderPass::ShadowsRenderPass()
        : RenderPass("Shadows") {}

    void ShadowsRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateRayTracingState(PSONames::Shadows, [](RayTracingStateProxy& state)
        {
            state.RayGenerationShaderFileName = "Shadows.hlsl";
            state.AddMissShader("Shadows.hlsl");
            state.ShaderConfig = HAL::RayTracingShaderConfig{ sizeof(float), 0 };
            state.GlobalRootSignatureName = RootSignatureNames::RayTracing;
            state.PipelineConfig = HAL::RayTracingPipelineConfig{ 1 };
        });
    }
     
    void ShadowsRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->NewTexture(ResourceNames::ShadowMask);
    } 

    void ShadowsRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::Shadows);

        ShadowsCBContent cbContent{};
        cbContent.ShadowMaskTextureIndex = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::ShadowMask);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        auto bvh = context->GetContent()->GetSceneGPUStorage()->TopAccelerationStructure().AccelerationStructureBuffer();
        context->GetCommandRecorder()->BindExternalBuffer(*bvh, 0, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->DispatchRays(context->GetDefaultRenderSurfaceDesc().Dimensions());
    }

}
