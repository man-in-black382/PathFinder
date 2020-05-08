#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct DenoiserReprojectionCBContent
    {
        uint32_t GBufferTextureIndex;
        uint32_t CurrentDepthTextureIndex;
        uint32_t PreviousDepthTextureIndex;
        uint32_t CurrentAccumulationCounterTextureIndex;
        uint32_t PreviousAccumulationCounterTextureIndex;
    };

    class DenoiserReprojectionRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        DenoiserReprojectionRenderPass();
        ~DenoiserReprojectionRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
