#pragma once

#include "../RenderSubPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"
#include "DownsamplingHelper.hpp"

#include <vector>

namespace PathFinder
{

    class DownsamplingRenderSubPass : public RenderSubPass<RenderPassContentMediator>
    { 
    public: 
        DownsamplingRenderSubPass(const std::string& name, uint64_t invocationIndex);
        DownsamplingRenderSubPass(const std::string& name);
        ~DownsamplingRenderSubPass() = default;

        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;

        void SetInvocationInputs(const std::vector<DownsamplingInvocationInputs>& inputs);
        void SetEnabled(bool enabled);

    private:
        Foundation::Name CombineNameAndInvocation(const std::string& name, uint64_t invocationIndex) const;

        bool mEnabled = true;
        std::vector<DownsamplingInvocationInputs> mInvocationInputs;
    };

}
