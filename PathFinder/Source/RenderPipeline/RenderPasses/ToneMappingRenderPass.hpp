#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct ToneMappingCBContent
    {
        uint32_t InputTexIdx;
        uint32_t OutputTexIdx;   
        uint32_t _Pad0;
        uint32_t _Pad1;
        // 16 byte boundary
        GTTonemappingParameterss TonemappingParams;
    };

    class ToneMappingRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        ToneMappingRenderPass();
        ~ToneMappingRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
