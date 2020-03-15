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
        uint32_t InputTextureIndex;
        uint32_t OutputTextureIndex;
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
        void BlurHalfResolution(RenderContext<RenderPassContentMediator>* context);
        void BlurQuadResolution(RenderContext<RenderPassContentMediator>* context);

        uint32_t mFullResBlurRadius = 8;
        uint32_t mHalfResBlurRadius = 16;
        uint32_t mQuadResBlurRadius = 32;
    };

}
