#include "DeferredShadowsRenderPass.hpp"

#include <Foundation/Halton.hpp>

namespace PathFinder
{

    DeferredShadowsRenderPass::DeferredShadowsRenderPass()
        : RenderPass("DeferredShadows") {} 

    void DeferredShadowsRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateRayTracingState(PSONames::DeferredShadows, [this](RayTracingStateProxy& state)
        {
            RayTracingStateProxy::HitGroupShaderFileNames meshHitGroup{};
            meshHitGroup.ClosestHitShaderFileName = "DeferredShadows.hlsl";

            // Order of hit group shaders must match hit group contribution of entities
            state.AddHitGroupShaders(meshHitGroup);

            state.RayGenerationShaderFileName = "DeferredShadows.hlsl";
            state.AddMissShader({ "DeferredShadows.hlsl" });
            state.ShaderConfig = HAL::RayTracingShaderConfig{ sizeof(float), sizeof(float) * 2 };
            state.GlobalRootSignatureName = RootSignatureNames::ShadingCommon;
            state.PipelineConfig = HAL::RayTracingPipelineConfig{ 1 };
        });
    }
     
    void DeferredShadowsRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    { 
        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingOutput);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingOutput);

        scheduler->ReadTexture(ResourceNames::GBufferAlbedoMetalness);
        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::GBufferTypeAndMaterialIndex);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
        scheduler->ReadTexture(ResourceNames::DeferredLightingRayPDFs);
        scheduler->ReadTexture(ResourceNames::DeferredLightingRayLightIntersectionPoints);

        scheduler->UseRayTracing();
    } 

    void DeferredShadowsRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DeferredShadows);

        const Scene* scene = context->GetContent()->GetScene();
        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();
        const Memory::Texture* blueNoiseTexture = scene->BlueNoiseTexture();

        auto resourceProvider = context->GetResourceProvider();

        DeferredShadowsCBContent cbContent{};

        cbContent.GBufferIndices.AlbedoMetalnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferAlbedoMetalness);
        cbContent.GBufferIndices.NormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughness);
        cbContent.GBufferIndices.MotionTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferMotionVector);
        cbContent.GBufferIndices.TypeAndMaterialTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferTypeAndMaterialIndex);
        cbContent.GBufferIndices.DepthStencilTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        cbContent.ShadowRayPDFsTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DeferredLightingRayPDFs);
        cbContent.ShadowRayIntersectionPointsTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::DeferredLightingRayLightIntersectionPoints);
        cbContent.StochasticShadowedOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingOutput);
        cbContent.StochasticUnshadowedOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingOutput);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->SetRootConstants(sceneStorage->CompressedLightPartitionInfo(), 0, 0);

        const Memory::Buffer* bvh = sceneStorage->TopAccelerationStructure().AccelerationStructureBuffer();
        const Memory::Buffer* lights = sceneStorage->LightTable();
        const Memory::Buffer* materials = sceneStorage->MaterialTable();

        if (bvh) context->GetCommandRecorder()->BindExternalBuffer(*bvh, 0, 0, HAL::ShaderRegister::ShaderResource);
        if (lights) context->GetCommandRecorder()->BindExternalBuffer(*lights, 1, 0, HAL::ShaderRegister::ShaderResource);
        if (materials) context->GetCommandRecorder()->BindExternalBuffer(*materials, 2, 0, HAL::ShaderRegister::ShaderResource);

        context->GetCommandRecorder()->DispatchRays(context->GetDefaultRenderSurfaceDesc().Dimensions());
    }

}
