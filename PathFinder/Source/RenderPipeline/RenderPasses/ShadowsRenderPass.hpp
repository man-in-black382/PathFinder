#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{
     
    class ShadowsRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        ShadowsRenderPass();
        ~ShadowsRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
