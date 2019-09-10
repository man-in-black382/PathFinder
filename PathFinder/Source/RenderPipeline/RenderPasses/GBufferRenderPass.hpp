#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct GBufferCBContent
    {
        
    };

    class GBufferRenderPass : public RenderPass  
    {
    public:
        GBufferRenderPass();

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext* context) override; 
    };

}
