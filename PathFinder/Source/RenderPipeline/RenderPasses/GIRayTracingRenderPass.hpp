#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct GIRayTracingCBContent
    {
        GPUIlluminanceField ProbeField;
        glm::vec4 Halton;
        glm::uvec2 BlueNoiseTexSize;
        uint32_t BlueNoiseTexIdx;
        uint32_t SkyTexIdx;
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
