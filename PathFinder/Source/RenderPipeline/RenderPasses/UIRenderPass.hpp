#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct UICBContent
    {
        glm::mat4 ProjectionMatrix;
        uint32_t UITextureSRVIndex;
    };

    // Separate root constants to version vertex/index 
    // offsets between draw calls
    struct UIRootConstants
    {
        uint32_t VertexBufferOffset;
        uint32_t IndexBufferOffset;
        int32_t TextureIdx;
        int32_t SamplerIdx;
    };

    class UIRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        UIRenderPass();
        ~UIRenderPass() = default;

        virtual void SetupRootSignatures(RootSignatureCreator* rootSignatureCreator) override;
        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
