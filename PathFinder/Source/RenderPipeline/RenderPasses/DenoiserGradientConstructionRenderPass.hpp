#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct DenoiserGradientConstructionCBContent
    {
        uint32_t ShadingShadowedTexIdx;
        uint32_t ShadingUnshadowedTexIdx;
        uint32_t ShadingShadowedHistoryTexIdx;
        uint32_t ShadingUnshadowedHistoryTexIdx;
        uint32_t ShadingShadowedGradientTexIdx;
        uint32_t ShadingUnshadowedGradientTexIdx;
    };

    class DenoiserGradientConstructionRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserGradientConstructionRenderPass();
        ~DenoiserGradientConstructionRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
