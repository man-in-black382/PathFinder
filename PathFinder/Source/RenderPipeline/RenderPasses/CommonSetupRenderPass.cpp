#include "CommonSetupRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    CommonSetupRenderPass::CommonSetupRenderPass()
        : RenderPass("CommonSetup", RenderPassPurpose::Setup) {}

    void CommonSetupRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::Downsampling, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "Downsampling.hlsl";
        });

        stateCreator->CreateComputeState(PSONames::SeparableBlur, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "SeparableBlur.hlsl";
        });
    }

    void CommonSetupRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
    }

}
