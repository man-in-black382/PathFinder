#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

namespace PathFinder
{

    struct RngSeedGenerationCBContent
    {
        uint32_t RngSeedTexIdx;
        uint32_t FrameIdx;
        uint32_t BlueNoiseTexSize;
    };

    class RngSeedGenerationRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        RngSeedGenerationRenderPass();
        ~RngSeedGenerationRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
