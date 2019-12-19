#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct UICBContent
    {
        
    };

    class UIRenderPass : public RenderPass  
    {
    public:
        UIRenderPass();
        ~UIRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext* context) override; 
    };

}
