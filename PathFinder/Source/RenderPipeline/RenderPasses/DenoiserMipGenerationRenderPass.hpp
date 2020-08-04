#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include "DownsamplingHelper.hpp"

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

    private:
        void GenerateMips(Foundation::Name resourceName, RenderContext<RenderPassContentMediator>* context, DownsamplingCBContent::Filter filter, DownsamplingStrategy strategy);
    };

}
