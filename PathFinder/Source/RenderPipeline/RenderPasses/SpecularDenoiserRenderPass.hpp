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
        uint32_t AccumulatedFramesCountTexIdx;
        uint32_t ShadowedShadingHistoryTexIdx;
        uint32_t UnshadowedShadingHistoryTexIdx;
        uint32_t CurrentShadowedShadingTexIdx;
        uint32_t CurrentUnshadowedShadingTexIdx;
        uint32_t ShadowedShadingDenoiseTargetTexIdx;
        uint32_t UnshadowedShadingDenoiseTargetTexIdx;
    };

    class SpecularDenoiserRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        SpecularDenoiserRenderPass();
        ~SpecularDenoiserRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
