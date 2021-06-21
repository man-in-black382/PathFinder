#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct GIProbeUpdateCBContent
    {
        GPUIlluminanceField ProbeField;
    };

    class GIProbeUpdateRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        GIProbeUpdateRenderPass();
        ~GIProbeUpdateRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
