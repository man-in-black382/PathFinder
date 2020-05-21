#include "CommonSetupRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    CommonSetupRenderPass::CommonSetupRenderPass()
        : RenderPass("CommonSetup", RenderPassPurpose::Setup) {}

    void CommonSetupRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::AveragindDownsampling, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "Downsampling.hlsl";
        });
    }

    void CommonSetupRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
    }

}
