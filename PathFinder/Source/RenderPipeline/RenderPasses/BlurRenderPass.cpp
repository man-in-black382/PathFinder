#include "BlurRenderPass.hpp"

namespace PathFinder
{

    BlurRenderPass::BlurRenderPass()
        : RenderPass("Blur") {}

    void BlurRenderPass::SetupPipelineStates(IShaderManager* shaderManager, IPipelineStateManager* psoManager)
    {
        auto pso = psoManager->CloneDefaultComputeState();
        pso.SetShaders(shaderManager->LoadShaders("Blur.hlsl")); 
        psoManager->StoreComputeState(PSONames::Blur, pso, RootSignatureNames::Universal);
    }

    void BlurRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->WillUseRootConstantBuffer<BlurCBContent>();
        scheduler->ReadTexture(ResourceNames::PlaygroundRenderTarget);
        scheduler->NewTexture(ResourceNames::BlurResult);
    }

    void BlurRenderPass::Render(RenderContext* context)
    {
        context->GetGraphicsDevice()->ApplyPipelineState(PSONames::Blur);
        context->GetGraphicsDevice()->SetViewport({ 1280, 720 });

        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<BlurCBContent>();
        cbContent->BlurRadius = 5;
        cbContent->Weights[0] = 0.2f;
        cbContent->Weights[1] = 0.2f;
        cbContent->Weights[2] = 0.2f;
        cbContent->Weights[3] = 0.2f;
        cbContent->Weights[4] = 0.2f;
        
        cbContent->InputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::PlaygroundRenderTarget);
        cbContent->OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::BlurResult);
    }

}
