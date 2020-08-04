#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "BlurCBContent.hpp"
#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct BloomDownscalingCBContent
    {
        uint32_t FullResSourceTexIdx;
        uint32_t HalfResDestinationTexIdx;
    };

    class BloomBlurRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        BloomBlurRenderPass();
        ~BloomBlurRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;

    private:
        void BlurFullResolution(RenderContext<RenderPassContentMediator>* context);
        void DownscaleAndBlurHalfResolution(RenderContext<RenderPassContentMediator>* context);
        void DownscaleAndBlurQuadResolution(RenderContext<RenderPassContentMediator>* context);
    };

}
