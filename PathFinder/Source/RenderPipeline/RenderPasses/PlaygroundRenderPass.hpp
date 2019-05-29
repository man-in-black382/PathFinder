#pragma once

#include "../RenderPass.hpp"

namespace PathFinder
{

    class PlaygroundRenderPass : public RenderPass
    {
    public:
        PlaygroundRenderPass();

        virtual void ScheduleResources(const IRenderPassScheduler* scheduler) override;

    };

}
