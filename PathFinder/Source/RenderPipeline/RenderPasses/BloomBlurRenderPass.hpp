#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct BloomBlurCBContent
    {
        static const int MaximumRadius = 64;

        std::array<float, MaximumRadius> Weights;
        glm::vec2 ImageSize;
        uint32_t IsHorizontal;
        uint32_t BlurRadius;
        uint32_t InputTexIdx;
        uint32_t OutputTexIdx;
    };

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
