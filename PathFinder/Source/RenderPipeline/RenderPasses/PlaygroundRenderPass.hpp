#pragma once

#include "../RenderPass.hpp"

namespace PathFinder
{

    class PlaygroundRenderPass : public RenderPass
    {
    public:
        PlaygroundRenderPass();


        virtual void ScheduleResources(IResourceScheduler* scheduler) override;
        virtual void Render(IResourceProvider* resourceProvider, GraphicsDevice* device) override;

    };

}
