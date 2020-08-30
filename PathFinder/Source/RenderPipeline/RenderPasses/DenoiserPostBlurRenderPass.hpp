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

    class DenoiserPostBlurRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserPostBlurRenderPass();
        ~DenoiserPostBlurRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
