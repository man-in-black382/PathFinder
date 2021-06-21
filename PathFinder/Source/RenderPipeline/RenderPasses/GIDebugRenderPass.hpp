#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct GIDebugCBContent
    {
        GPUIlluminanceField ProbeField;
        int32_t ExplicitProbeIndex = -1;
    };

    class GIDebugRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        GIDebugRenderPass();
        ~GIDebugRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
