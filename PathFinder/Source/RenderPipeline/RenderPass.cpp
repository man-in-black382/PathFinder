#include "RenderPass.hpp"

namespace PathFinder
{

    RenderPass::RenderPass(Foundation::Name name, Purpose runMode)
        : mName{ name }, mPurpose{ runMode } {}

    RenderPass::~RenderPass() {}

    void RenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator) {}

    void RenderPass::ScheduleResources(ResourceScheduler* scheduler) {}

    void RenderPass::Render(RenderContext* context) {}

}
