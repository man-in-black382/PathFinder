#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"
#include "DownsamplingHelper.hpp"
#include "DownsamplingRenderSubPass.hpp"

namespace PathFinder
{

    class DenoiserMipGenerationRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserMipGenerationRenderPass();
        ~DenoiserMipGenerationRenderPass() = default;

        virtual void ScheduleSubPasses(SubPassScheduler<RenderPassContentMediator>* scheduler) override;

    private:
        std::vector<std::unique_ptr<DownsamplingRenderSubPass>> mDownsamplingSubPasses;
    };

}
