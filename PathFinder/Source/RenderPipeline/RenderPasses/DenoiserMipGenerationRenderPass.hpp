#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

namespace PathFinder
{

    class DenoiserMipGenerationRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserMipGenerationRenderPass();
        ~DenoiserMipGenerationRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
