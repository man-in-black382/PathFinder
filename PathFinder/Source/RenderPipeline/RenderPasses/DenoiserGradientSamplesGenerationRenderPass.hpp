#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct DenoiserGradientSamplesGenerationCBContent
    {
        glm::uvec2 DispatchGroupCount;
        uint32_t ViewDepthPrevTexIdx;
        uint32_t AlbedoMetalnessPrevTexIdx;
        uint32_t NormalRoughnessPrevTexIdx;
        uint32_t ReprojectedTexelIndicesTexIdx;
        uint32_t StochasticShadowedShadingPrevTexIdx;
        uint32_t StochasticRngSeedsPrevTexIdx;
        uint32_t GradientSamplePositionsPrevTexIdx;
        uint32_t StochasticRngSeedsOutputTexIdx;
        uint32_t GradientSamplePositionsOutputTexIdx;
        uint32_t GradientSamplesOutputTexIdx;
        uint32_t AlbedoMetalnessOutputTexIdx;
        uint32_t NormalRoughnessOutputTexIdx;
        uint32_t ViewDepthOutputTexIdx;
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
