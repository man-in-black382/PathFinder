#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct GIProbeIndirectionTableUpdateCBContent
    {
        GPUIrradianceField ProbeField;
    };

    class GIProbeIndirectionTableUpdateRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        GIProbeIndirectionTableUpdateRenderPass();
        ~GIProbeIndirectionTableUpdateRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
