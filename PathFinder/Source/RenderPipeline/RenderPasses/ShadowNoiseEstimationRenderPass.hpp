#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    //struct ToneMappingCBContent
    //{
    //    uint32_t InputTextureIndex;
    //    uint32_t OutputTextureIndex;   
    //    uint32_t _Padding0;
    //    uint32_t _Padding1;
    //    // 16 byte boundary
    //    GTTonemappingParameterss TonemappingParams;
    //};

    class ShadowNoiseEstimationRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        ShadowNoiseEstimationRenderPass();
        ~ShadowNoiseEstimationRenderPass() = default;

        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator) override;
        virtual void ScheduleResources(ResourceScheduler* scheduler) override;
        virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
