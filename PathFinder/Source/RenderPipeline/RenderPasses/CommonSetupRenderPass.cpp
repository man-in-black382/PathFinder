#include "CommonSetupRenderPass.hpp"

#include "../Foundation/GaussianFunction.hpp"

namespace PathFinder
{

    CommonSetupRenderPass::CommonSetupRenderPass()
        : RenderPass("CommonSetup", RenderPassPurpose::Setup) {}

    void CommonSetupRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature rayTracingSinature = stateCreator->CloneBaseRootSignature();

        rayTracingSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 }); // Scene BVH | t0 - s0
        rayTracingSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 1, 0 }); // Instance Table Structured Buffer | t1 - s0
        rayTracingSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 2, 0 }); // Unified Vertex Buffer | t2 - s0
        rayTracingSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 3, 0 }); // Unified Index Buffer | t3 - s0

        stateCreator->StoreRootSignature(RootSignatureNames::RayTracing, std::move(rayTracingSinature));
    }

    void CommonSetupRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
    }

}
