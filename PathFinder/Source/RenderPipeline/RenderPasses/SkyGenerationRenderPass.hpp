#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"
#include <Scene/SceneGPUTypes.hpp>

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct SkyGenerationCBContent
    {
        glm::vec3 SunDirection;
        uint32_t SkyTexIdx;
        glm::uvec2 SkyTexSize;
        glm::uvec2 DispatchGroupCount;
        ArHosekSkyModelStateGPU SkyStateR;
        ArHosekSkyModelStateGPU SkyStateG;
        ArHosekSkyModelStateGPU SkyStateB;
    };

    class SkyGenerationRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        SkyGenerationRenderPass();
        ~SkyGenerationRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
