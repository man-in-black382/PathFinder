#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct PlaygroundCBContent
    {
        glm::mat4 cameraMat;
    };

    class PlaygroundRenderPass : public RenderPass 
    {
    public:
        PlaygroundRenderPass();

        virtual void SetupPipelineStates(IShaderManager* shaderManager, IPipelineStateManager* psoManager) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext* context) override;
    };

}
