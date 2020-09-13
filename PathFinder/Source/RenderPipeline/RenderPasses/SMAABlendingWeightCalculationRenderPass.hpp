#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct SMAABlendingWeightCalculationCBContent
    {
        uint32_t EdgesTexIdx;
        uint32_t AreaTexIdx;
        uint32_t SearchTexIdx;
    };

    class SMAABlendingWeightCalculationRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        SMAABlendingWeightCalculationRenderPass();
        ~SMAABlendingWeightCalculationRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
