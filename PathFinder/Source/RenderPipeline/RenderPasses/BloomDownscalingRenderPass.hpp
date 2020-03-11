#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct BloomDownscalingCBContent
    {
        glm::vec2 SourceTextureInverseDimensions;
        uint32_t SourceTextureSRIndex;
        uint32_t HalfSizeDestinationTextureUAIndex;
        uint32_t QuadSizeDestinationTextureUAIndex;
    };

    class BloomDownscalingRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        BloomDownscalingRenderPass();
        ~BloomDownscalingRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
