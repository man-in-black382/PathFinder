#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/vec2.hpp>

namespace PathFinder
{

    struct BloomCompositionCBContent
    {
        glm::vec2 InverseTextureDimensions;
        uint32_t DeferredLightingOutputTextureIndex;
        uint32_t BloomBlurOutputTextureIndex;
        uint32_t CompositionOutputTextureIndex;
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
