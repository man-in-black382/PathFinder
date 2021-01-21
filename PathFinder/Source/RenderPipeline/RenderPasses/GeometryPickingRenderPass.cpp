#include "GeometryPickingRenderPass.hpp"

#include <Foundation/Halton.hpp>

namespace PathFinder
{

    GeometryPickingRenderPass::GeometryPickingRenderPass()
        : RenderPass("GeometryPicking") {} 

    void GeometryPickingRenderPass::SetupRootSignatures(RootSignatureCreator* rootSignatureCreator)
    {
        rootSignatureCreator->CreateRootSignature(RootSignatureNames::GeometryPicking, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddShaderResourceBufferParameter(0, 0); // Scene BVH | t0 - s0
            signatureProxy.AddUnorderedAccessBufferParameter(0, 0); // Intersection Info Buffer | u0 - s0
        });
    }

    void GeometryPickingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateRayTracingState(PSONames::GeometryPicking, [this](RayTracingStateProxy& state)
        {
            RayTracingStateProxy::HitGroupShaderFileNames meshHitGroup{};
            meshHitGroup.ClosestHitShaderFileName = "GeometryPicking.hlsl";
            meshHitGroup.ClosestHitShaderEntryPoint = "MeshRayClosestHit";

            RayTracingStateProxy::HitGroupShaderFileNames lightHitGroup{};
            lightHitGroup.ClosestHitShaderFileName = "GeometryPicking.hlsl";
            lightHitGroup.ClosestHitShaderEntryPoint = "LightRayClosestHit";

            // Order of hit group shaders must match hit group contribution of entities
            state.AddHitGroupShaders(meshHitGroup);
            state.AddHitGroupShaders(lightHitGroup);

            state.RayGenerationShaderFileName = "GeometryPicking.hlsl";
            
            state.ShaderConfig = HAL::RayTracingShaderConfig{ sizeof(float) * 2, sizeof(float) * 2 };
            state.GlobalRootSignatureName = RootSignatureNames::GeometryPicking;
            state.PipelineConfig = HAL::RayTracingPipelineConfig{ 1 };
        });
    }
     
    void GeometryPickingRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    { 
        scheduler->NewBuffer(ResourceNames::PickedGeometryInfo, NewBufferProperties<uint32_t>{ 1 });
        scheduler->Export(ResourceNames::PickedGeometryInfo);
        scheduler->UseRayTracing();
        scheduler->ExecuteOnQueue(RenderPassExecutionQueue::AsyncCompute);
    } 

    void GeometryPickingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        const Scene* scene = context->GetContent()->GetScene();
        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();
        const Memory::Buffer* bvh = sceneStorage->TopAccelerationStructure().AccelerationStructureBuffer();

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GeometryPicking);

        GeometryPickingCBContent cbContent{};
        cbContent.MousePosition = context->GetContent()->GetInput()->MousePosition();

        if (bvh) context->GetCommandRecorder()->BindExternalBuffer(*bvh, 0, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindBuffer(ResourceNames::PickedGeometryInfo, 0, 0, HAL::ShaderRegister::UnorderedAccess);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->DispatchRays({ 1 });
    }

}
