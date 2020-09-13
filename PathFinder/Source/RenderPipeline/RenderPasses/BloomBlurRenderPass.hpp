#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "BlurCBContent.hpp"
#include "PipelineNames.hpp"
#include "DownsamplingHelper.hpp"
#include "DownsamplingRenderSubPass.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    class BloomBlurRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        BloomBlurRenderPass();
        ~BloomBlurRenderPass() = default;

        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void ScheduleSubPasses(SubPassScheduler<RenderPassContentMediator>* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;

    private:
        void Blur(RenderContext<RenderPassContentMediator>* context, uint8_t mipLevel, uint64_t radius, float sigma);

        std::unique_ptr<DownsamplingRenderSubPass> mMipGenerationSubPass;
    };

}
