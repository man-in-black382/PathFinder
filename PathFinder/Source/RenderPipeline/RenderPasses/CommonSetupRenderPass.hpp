#pragma once

#include "../RenderPass.hpp"
#include "../RenderPassContentMediator.hpp"

#include "PipelineNames.hpp"

#include <glm/mat4x4.hpp>

namespace PathFinder
{

    class CommonSetupRenderPass : public RenderPass<RenderPassContentMediator>
    { 
    public: 
        CommonSetupRenderPass();
        ~CommonSetupRenderPass() = default;

        virtual void SetupRootSignatures(RootSignatureCreator* rootSignatureCreator) override;
        virtual void SetupPipelineStates(PipelineStateCreator* stateCreator) override;
        virtual void ScheduleSamplers(SamplerCreator* samplerCreator) override;
    };

}
