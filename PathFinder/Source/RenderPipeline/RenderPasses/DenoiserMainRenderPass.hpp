#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "GBufferTextureIndices.hpp"
#include "PipelineNames.hpp"

namespace PathFinder
{

    struct SpecularDenoiserCBContent
    {
        GBufferTextureIndices GBufferIndices;
        glm::uvec2 DispatchGroupCount;
        uint32_t AccumulatedFramesCountTexIdx;
        uint32_t ShadowedShadingHistoryTexIdx;
        uint32_t UnshadowedShadingHistoryTexIdx;
        uint32_t CurrentShadowedShadingTexIdx;
        uint32_t CurrentUnshadowedShadingTexIdx;
        uint32_t ShadowedShadingDenoiseTargetTexIdx;
        uint32_t UnshadowedShadingDenoiseTargetTexIdx;
        uint32_t AccumulatedFramesCountPatchedTargetTexIdx;
        uint32_t PrimaryGradientTexIdx;
        uint32_t SecondaryGradientTexIdx;
    };

    class DenoiserMainRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserMainRenderPass();
        ~DenoiserMainRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
