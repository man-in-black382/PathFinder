#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{
     
    struct DisplacementDistanceMapGenerationCBContent
    {
        uint32_t DisplacementMapSRVIndex = 0;
        uint32_t DistanceAltasIndirectionMapUAVIndex = 0;
        uint32_t DistanceAltasUAVIndex = 0;
    };

    class DisplacementDistanceMapRenderPass : public RenderPass 
    {
    public:
        DisplacementDistanceMapRenderPass();
        ~DisplacementDistanceMapRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext* context) override;
    };

}
