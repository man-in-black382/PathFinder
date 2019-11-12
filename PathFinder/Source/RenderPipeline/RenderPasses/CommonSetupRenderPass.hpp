#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    class CommonSetupRenderPass : public RenderPass
    { 
    public: 
        CommonSetupRenderPass();
        ~CommonSetupRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
    };

}
