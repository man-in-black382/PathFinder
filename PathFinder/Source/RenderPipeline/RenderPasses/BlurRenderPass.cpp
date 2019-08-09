#include "BlurRenderPass.hpp"

namespace PathFinder
{

    BlurRenderPass::BlurRenderPass()
        : RenderPass("Blur") {}

    void BlurRenderPass::SetupPipelineStates(IShaderManager* shaderManager, IPipelineStateManager* psoManager)
    {
        auto pso = psoManager->CloneDefaultComputeState();
        pso.SetShaders(shaderManager->LoadShaders("Blur.hlsl")); 
        psoManager->StoreComputeState(PSONames::Blur, pso);
    }

    void BlurRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->WillUseRootConstantBuffer<BlurCBContent>();
    }

    void BlurRenderPass::Render(RenderContext* context)
    {
        context->GraphicsDevice()->ApplyPipelineState(PSONames::Blur);
        context->GraphicsDevice()->SetViewport({ 1280, 720 });

        auto cbContent = context->ConstantsUpdater()->UpdateRootConstantBuffer<BlurCBContent>();
        cbContent->Radius = 5;

        
    }

}
