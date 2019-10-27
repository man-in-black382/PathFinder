#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct DeferredLightingCBContent
    {
        uint32_t GBufferMaterialDataTextureIndex;
    };

    class DeferredLightingRenderPass : public RenderPass  
    {
    public:
        DeferredLightingRenderPass();

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext* context) override; 
    };

}
