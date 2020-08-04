#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct GBufferCBContent
    {
        uint32_t InstanceTableIndex;
        uint32_t ParallaxCounterTextureUAVIndex;
    };

    class GBufferRenderPass : public RenderPass<RenderPassContentMediator>
    {
    public:
        GBufferRenderPass();
        ~GBufferRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override; 
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;

    private:
        void RenderMeshes(RenderContext<RenderPassContentMediator>* context);
        void RenderLights(RenderContext<RenderPassContentMediator>* context);
    };

}
