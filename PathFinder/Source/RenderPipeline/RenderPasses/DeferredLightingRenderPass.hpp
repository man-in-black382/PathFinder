#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct DeferredLightingCBContent
    {
        uint32_t GBufferMaterialDataTextureIndex;
        uint32_t GBufferDepthTextureIndex;
        uint32_t MainOutputTextureIndex;
        uint32_t OversaturatedOutputTextureIndex;
    };

    class DeferredLightingRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        DeferredLightingRenderPass();
        ~DeferredLightingRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
