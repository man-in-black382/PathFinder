#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct DenoiserReprojectionCBContent
    {
        uint32_t GBufferNormalRoughnessTexIdx;
        uint32_t DepthTexIdx;
        uint32_t CurrentViewDepthTexIdx;
        uint32_t PreviousViewDepthTexIdx;
        uint32_t CurrentAccumulationCounterTexIdx;
        uint32_t PreviousAccumulationCounterTexIdx;
        uint32_t ShadowedShadingTexIdx;
        uint32_t UnshadowedShadingTexIdx;
        uint32_t ShadowedShadingHistoryTexIdx;
        uint32_t UnshadowedShadingHistoryTexIdx;
        uint32_t ShadowedShadingReprojectionTargetTexIdx;
        uint32_t UnshadowedShadingReprojectionTargetTexIdx;
        uint32_t ShadingGradientTexIdx;
        uint32_t ShadingGradientNormFactorTexIdx;
    };

    class DenoiserReprojectionRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserReprojectionRenderPass();
        ~DenoiserReprojectionRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
