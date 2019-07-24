#pragma once

#include "../RenderPass.hpp"

namespace PathFinder
{

    class PlaygroundRenderPass : public RenderPass
    {
    public:
        PlaygroundRenderPass();

        virtual void SetupPipelineStates(IShaderManager* shaderManager, IPipelineStateManager* psoManager) override;
        virtual void ScheduleResources(IResourceScheduler* scheduler) override;
        virtual void Render(IResourceProvider* resourceProvider, GraphicsDevice* device) override;
    };

}
