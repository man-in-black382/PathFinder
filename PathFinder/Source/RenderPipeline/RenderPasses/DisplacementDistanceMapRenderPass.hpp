#pragma once

#include "../RenderPass.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace PathFinder
{
     
    struct DisplacementDistanceMapGenerationCBContent
    {
        // Always use vec4 to avoid constant buffer padding issues
        glm::uvec4 DisplacementMapSize;
        glm::uvec4 DistanceAtlasIndirectionMapSize;
        uint32_t DisplacementMapSRVIndex = 0;
        uint32_t DistanceAltasIndirectionMapUAVIndex = 0;
        uint32_t DistanceAltasUAVIndex = 0;
        uint32_t ReadOnlyJFAConesIndirectionUAVIndex = 0;
        uint32_t WriteOnlyJFAConesIndirectionUAVIndex = 0;
        uint32_t ReadOnlyJFAConesUAVIndex = 0;
        uint32_t WriteOnlyJFAConesUAVIndex = 0;
        uint32_t FloodStep = 0;
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
