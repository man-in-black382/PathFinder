#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct BlurCBContent
    {
        static const int MaximumRadius = 64;

        uint32_t BlurRadius; 
        float Weights[MaximumRadius + 1];
        uint32_t InputTextureIndex;
        uint32_t OutputTextureIndex;
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
