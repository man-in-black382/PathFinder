#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct ShadingCBContent
    {
        static const uint32_t MaxSupportedLights = 4;

        glm::vec4 HaltonSequence[MaxSupportedLights];
        // 16 byte boundary
        uint32_t BlueNoiseTextureIndex;
        uint32_t AnalyticalLuminanceOutputTextureIndex;
        uint32_t StochasticUnshadowedLuminanceOutputTextureIndex;
        uint32_t StochasticShadowedLuminanceOutputTextureIndex;
        // 16 byte boundary
        glm::uvec2 BlueNoiseTextureSize;
        uint32_t GBufferMaterialDataTextureIndex;
        uint32_t GBufferDepthTextureIndex;
        // 16 byte boundary
        GPULightTablePartitionInfo LightOffsets;
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
        uint64_t mFrameNumber = 0;
    };

}
