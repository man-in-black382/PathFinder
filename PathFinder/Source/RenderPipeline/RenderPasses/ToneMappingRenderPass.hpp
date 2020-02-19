#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct ToneMappingCBContent
    {
        uint32_t InputTextureIndex;
        uint32_t OutputTextureIndex;   
        uint32_t _Padding0;
        uint32_t _Padding1;
        // 16 byte boundary
        GTTonemappingParams TonemappingParams;
    };

    class ToneMappingRenderPass : public RenderPass
    { 
    public: 
        ToneMappingRenderPass();
        ~ToneMappingRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext* context) override;
    };

}
