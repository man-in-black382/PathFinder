#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "GBufferTextureIndices.hpp"
#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct ShadingCBContent
    {
        GBufferTextureIndices GBufferIndices;
        glm::vec4 Halton;
        // 16 byte boundary
        uint32_t BlueNoiseTexIdx;
        uint32_t AnalyticOutputTexIdx;
        uint32_t StochasticShadowedOutputTexIdx;
        uint32_t StochasticUnshadowedOutputTexIdx;
        // 16 byte boundary
        glm::uvec2 BlueNoiseTextureSize;
        uint32_t RngSeedsTexIdx;
        uint32_t FrameNumber;
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
