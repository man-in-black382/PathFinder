#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct BlurCBContent
    {
        static const int MaximumRadius = 64;

        std::array<float, MaximumRadius> Weights;

        uint32_t BlurRadius;
        uint32_t InputTextureIndex;
        uint32_t OutputTextureIndex;
        
    };

    class BlurRenderPass : public RenderPass
    { 
    public: 
        BlurRenderPass();

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext* context) override;
    };

}
