#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct BackBufferOutputPassData
    {
        uint32_t SourceTextureIndex;
    };
     
    class BackBufferOutputPass : public RenderPass<RenderPassContentMediator> 
    {
    public:
        BackBufferOutputPass();
        ~BackBufferOutputPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
