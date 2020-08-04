#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct DenoiserPostStabilizationCBContent
    {
        uint32_t InputTexIdx;
        uint32_t OutputTexIdx;
    };

    class DenoiserPostStabilizationRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserPostStabilizationRenderPass();
        ~DenoiserPostStabilizationRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
