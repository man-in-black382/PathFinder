#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "GBufferTextureIndices.hpp"
#include "PipelineNames.hpp"

namespace PathFinder
{

    struct DeferredShadowsCBContent
    {
        GBufferTextureIndices GBufferIndices;
        uint32_t ReprojectedTexelIndicesTexIdx;
        uint32_t DenoiserGradientSamplePositionsTexIdx;
        uint32_t ShadowRayPDFsTexIdx;
        uint32_t ShadowRayIntersectionPointsTexIdx;
        uint32_t StochasticShadowedOutputTexIdx;
        uint32_t StochasticUnshadowedOutputTexIdx;
        uint32_t BlueNoiseTexSize;
        uint32_t BlueNoiseTexDepth;
        uint32_t BlueNoiseTexIdx;
        uint32_t FrameNumber;
    };

    class DeferredShadowsRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        DeferredShadowsRenderPass();
        ~DeferredShadowsRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
