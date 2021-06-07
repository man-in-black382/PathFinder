#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct DenoiserReprojectionCBContent
    {
        glm::uvec2 DispatchGroupCount;
        uint32_t GBufferNormalRoughnessTexIdx;
        uint32_t MotionTexIdx;
        uint32_t DepthStencilTexIdx;
        uint32_t PreviousViewDepthTexIdx;
        uint32_t StochasticShadowedShadingPrevTexIdx;
        uint32_t CurrentAccumulationCounterTexIdx;
        uint32_t PreviousAccumulationCounterTexIdx;
        uint32_t ShadowedShadingDenoisedHistoryTexIdx;
        uint32_t UnshadowedShadingDenoisedHistoryTexIdx;
        uint32_t ShadowedShadingReprojectionTargetTexIdx;
        uint32_t UnshadowedShadingReprojectionTargetTexIdx;
        uint32_t ReprojectedTexelIndicesTargetTexIdx;
    };

    class DenoiserReprojectionRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserReprojectionRenderPass();
        ~DenoiserReprojectionRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
