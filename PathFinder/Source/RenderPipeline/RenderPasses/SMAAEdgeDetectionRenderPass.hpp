#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct SMAAEdgeDetectionCBContent
    {
        uint32_t InputTexIdx;
    };

    class SMAAEdgeDetectionRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        SMAAEdgeDetectionRenderPass();
        ~SMAAEdgeDetectionRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
