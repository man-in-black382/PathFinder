#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct SMAANeighborhoodBlendingCBContent
    {
        uint32_t InputImageTexIdx;
        uint32_t BlendingWeightsTexIdx;
    };

    class SMAANeighborhoodBlendingRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        SMAANeighborhoodBlendingRenderPass();
        ~SMAANeighborhoodBlendingRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
