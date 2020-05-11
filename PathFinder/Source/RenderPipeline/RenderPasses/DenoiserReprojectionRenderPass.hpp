#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct DenoiserReprojectionCBContent
    {
        uint32_t GBufferNormalTextureIndex;
        uint32_t DepthTextureIndex;
        uint32_t CurrentViewDepthTextureIndex;
        uint32_t PreviousViewDepthTextureIndex;
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
