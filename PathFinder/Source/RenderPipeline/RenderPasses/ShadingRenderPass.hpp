#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "GBufferTextureIndices.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct ShadingCBContent
    {
        static const uint32_t MaxSupportedLights = 4;

        GBufferTextureIndices GBufferIndices;
        // 16 byte boundary
        glm::vec4 HaltonSequence[MaxSupportedLights];
        // 16 byte boundary
        uint32_t BlueNoiseTextureIndex;
        uint32_t AnalyticOutputTextureIndex;
        uint32_t StochasticShadowedOutputTextureIndex;
        uint32_t StochasticUnshadowedOutputTextureIndex;
        // 16 byte boundary
        glm::uvec2 BlueNoiseTextureSize;
        uint32_t __Pad0;
        uint32_t __Pad1;
    };

    class ShadingRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        ShadingRenderPass();
        ~ShadingRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;

    private:
        uint32_t CompressLightPartitionInfo(const GPULightTablePartitionInfo& info) const;
    };

}
