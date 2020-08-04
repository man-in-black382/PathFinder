#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

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
        uint32_t FloodStep = 0;
    };

    class DisplacementDistanceMapRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        DisplacementDistanceMapRenderPass();
        ~DisplacementDistanceMapRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;

    private:
        struct JFACones
        {
            // 3 first floats for JFA seed position and 4th float for distance to it
            // 8 directions
            glm::vec4 Cone[8];
        };
    };

}
