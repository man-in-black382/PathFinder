#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct DenoiserForwardProjectionCBContent
    {
        glm::uvec2 DispatchGroupCount;
        uint32_t GBufferViewDepthPrevTexIdx;
        uint32_t StochasticShadowedShadingPrevTexIdx;
        uint32_t StochasticUnshadowedShadingPrevTexIdx;
        uint32_t StochasticRngSeedsPrevTexIdx;
        uint32_t GradientSamplePositionsPrevTexIdx;
        uint32_t StochasticRngSeedsTexIdx;
        uint32_t GradientSamplePositionsTexIdx;
        uint32_t GradientTexIdx;
        uint32_t FrameNumber;
    };

    class DenoiserForwardProjectionRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserForwardProjectionRenderPass();
        ~DenoiserForwardProjectionRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
