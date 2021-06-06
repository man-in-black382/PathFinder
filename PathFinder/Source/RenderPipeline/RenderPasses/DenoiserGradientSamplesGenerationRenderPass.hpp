#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct DenoiserGradientSamplesGenerationCBContent
    {
        glm::uvec2 DispatchGroupCount;
        uint32_t DepthStencilTexIdx;
        uint32_t MotionTexIdx;
        uint32_t NormalRoughnessTexIdx;
        uint32_t GBufferViewDepthPrevTexIdx;
        uint32_t ShadowedShadingRawPrevTexIdx;
        uint32_t StochasticRngSeedsPrevTexIdx;
        uint32_t GradientSamplePositionsPrevTexIdx;
        uint32_t StochasticRngSeedsOutputTexIdx;
        uint32_t GradientSamplePositionsOutputTexIdx;
        uint32_t GradientSamplesOutputTexIdx;
        uint32_t FrameNumber;
    };

    class DenoiserGradientSamplesGenerationRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserGradientSamplesGenerationRenderPass();
        ~DenoiserGradientSamplesGenerationRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
