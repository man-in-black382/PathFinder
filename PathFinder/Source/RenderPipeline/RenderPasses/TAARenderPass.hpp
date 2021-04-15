#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct TAACBContent
    {
        uint32_t VarianceSourceTexIdx;
        uint32_t InputTexIdx;
        uint32_t OutputTexIdx;
    };

    class TAARenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        TAARenderPass();
        ~TAARenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
