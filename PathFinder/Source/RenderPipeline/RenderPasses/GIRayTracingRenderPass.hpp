#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct GIRayTracingCBContent
    {
        GPUIrradianceField ProbeField;
        glm::vec4 Halton;
        uint32_t BlueNoiseTexIdx;
        glm::uvec2 BlueNoiseTexSize;
    };

    class GIRayTracingRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        GIRayTracingRenderPass();
        ~GIRayTracingRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
