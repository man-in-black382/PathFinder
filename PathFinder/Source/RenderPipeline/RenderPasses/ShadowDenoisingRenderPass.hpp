#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct ShadowDenoisingCBContent
    {
        uint32_t NoiseEstimationTextureIndex;
        uint32_t AnalyticLuminanceTextureIndex;
        uint32_t StochasticShadowedLuminanceTextureIndex;
        uint32_t StochasticUnshadowedLuminanceTextureIndex;
        // 16 byte boundary
        uint32_t GBufferTextureIndex;
        uint32_t DepthTextureIndex;
        uint32_t IntermediateOutput0TextureIndex;
        uint32_t IntermediateOutput1TextureIndex;
        // 16 byte boundary
        glm::vec2 ImageSize;
        uint32_t FinalOutputTextureIndex;
        float MaximumLightsLuminance;
        // 16 byte boundary
        uint32_t IsHorizontal;
    };

    class ShadowDenoisingRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        ShadowDenoisingRenderPass();
        ~ShadowDenoisingRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
