#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    /* struct DenoiserPostStabilizationCBContent
     {
         uint32_t InputTexIdx;
         uint32_t OutputTexIdx;
     };*/

    class DenoiserGradientGuidedBlurRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserGradientGuidedBlurRenderPass();
        ~DenoiserGradientGuidedBlurRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
