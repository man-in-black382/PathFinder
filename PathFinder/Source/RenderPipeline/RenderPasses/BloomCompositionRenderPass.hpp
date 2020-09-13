#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/vec2.hpp>

namespace PathFinder
{

    struct BloomCompositionCBContent
    {
        glm::vec2 InverseTextureDimensions;
        uint32_t CombinedShadingTexIdx;
        uint32_t BloomBlurOutputTexIdx;
        uint32_t CompositionOutputTexIdx;
        uint32_t SmallBloomWeight;
        uint32_t MediumBloomWeight;
        uint32_t LargeBloomWeight;
    };

    class BloomCompositionRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        BloomCompositionRenderPass();
        ~BloomCompositionRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
