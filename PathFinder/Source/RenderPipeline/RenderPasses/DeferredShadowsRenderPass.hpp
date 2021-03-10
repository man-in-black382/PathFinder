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
        // 16 byte boundary
        uint32_t ShadowRayPDFsTexIdx;
        uint32_t ShadowRayIntersectionPointsTexIdx;
        uint32_t StochasticShadowedOutputTexIdx;
        uint32_t StochasticUnshadowedOutputTexIdx;
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
