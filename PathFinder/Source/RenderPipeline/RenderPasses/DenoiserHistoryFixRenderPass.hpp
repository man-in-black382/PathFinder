#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct DenoiserHistoryFixCBContent
    {
        uint32_t GBufferNormalRoughnessTexIdx;
        uint32_t ViewDepthTexIdx;
        uint32_t AccumulationCounterTexIdx;
        uint32_t ShadowedShadingFixedTexIdx;
        uint32_t UnshadowedShadingFixedTexIdx;
        uint32_t ShadowedShadingPreBlurredTexIdx;
        uint32_t UnshadowedShadingPreBlurredTexIdx;
    };

    class DenoiserHistoryFixRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserHistoryFixRenderPass();
        ~DenoiserHistoryFixRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
