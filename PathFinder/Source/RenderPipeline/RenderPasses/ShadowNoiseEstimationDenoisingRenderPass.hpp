#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct ShadowNoiseEstimationDenoisingCBContent
    {
        uint32_t NoiseEstimationTextureIndex;
        uint32_t OutputTextureIndex;
    };

    class ShadowNoiseEstimationDenoisingRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        ShadowNoiseEstimationDenoisingRenderPass();
        ~ShadowNoiseEstimationDenoisingRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
