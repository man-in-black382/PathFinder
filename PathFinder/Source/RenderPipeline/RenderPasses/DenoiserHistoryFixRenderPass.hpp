#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct DenoiserHistoryFixCBContent
    {
        uint32_t GBufferNormalRoughnessTexIdx;
        uint32_t ViewDepthTexIdx;
        uint32_t AccumulationCounterTexIdx;
        uint32_t ShadowedShadingMip0TexIdx;
        uint32_t UnshadowedShadingMip0TexIdx;
        uint32_t ShadowedShadingTailMipsTexIdx;
        uint32_t UnshadowedShadingTailMipsTexIdx;
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
