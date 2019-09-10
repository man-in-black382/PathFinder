#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct BackBufferOutputPassData
    {
        uint32_t SourceTextureIndex;
    };
     
    class BackBufferOutputPass : public RenderPass 
    {
    public:
        BackBufferOutputPass();

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext* context) override;
    };

}
