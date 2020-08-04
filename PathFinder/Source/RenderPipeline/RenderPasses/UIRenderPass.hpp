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
    };

    class UIRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        UIRenderPass();
        ~UIRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
