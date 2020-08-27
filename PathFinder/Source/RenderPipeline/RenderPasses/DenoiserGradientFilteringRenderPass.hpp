#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

     struct DenoiserGradientFilteringCBContent
     {
         glm::uvec2 ImageSize;
         uint32_t InputTexIdx;
         uint32_t OutputTexIdx;
         uint32_t CurrentIteration;
     };

    class DenoiserGradientFilteringRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserGradientFilteringRenderPass();
        ~DenoiserGradientFilteringRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
