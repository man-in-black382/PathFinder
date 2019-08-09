#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct BlurCBContent
    {
        uint32_t Radius;
    };

    class BlurRenderPass : public RenderPass
    {
    public:
        BlurRenderPass();

        virtual void SetupPipelineStates(IShaderManager* shaderManager, IPipelineStateManager* psoManager) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext* context) override;
    };

}
