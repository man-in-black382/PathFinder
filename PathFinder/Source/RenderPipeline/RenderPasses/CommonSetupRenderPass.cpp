#include "CommonSetupRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    CommonSetupRenderPass::CommonSetupRenderPass()
        : RenderPass("CommonSetup", RenderPassPurpose::Setup) {}

    void CommonSetupRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        rootSignatureCreator->CreateRootSignature(RootSignatureNames::RayTracing, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddShaderResourceBufferParameter(0, 0); // Scene BVH | t0 - s0
            signatureProxy.AddShaderResourceBufferParameter(1, 0); // Instance Table Structured Buffer | t1 - s0
            signatureProxy.AddShaderResourceBufferParameter(2, 0); // Unified Vertex Buffer | t2 - s0
            signatureProxy.AddShaderResourceBufferParameter(3, 0); // Unified Index Buffer | t3 - s0
            signatureProxy.AddShaderResourceBufferParameter(4, 0); // Light Table | t4 - s0
            signatureProxy.AddShaderResourceBufferParameter(5, 0); // Light Table | t5 - s0
        });
    }

    void CommonSetupRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
    }

}
