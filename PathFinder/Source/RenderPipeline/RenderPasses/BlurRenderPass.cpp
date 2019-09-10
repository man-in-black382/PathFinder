#include "BlurRenderPass.hpp"

#include "../Foundation/GaussianFunction.hpp"

namespace PathFinder
{

    BlurRenderPass::BlurRenderPass()
        : RenderPass("Blur") {}

    void BlurRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::Blur, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"Blur.hlsl";
        });
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
        cbContent->BlurRadius = 20;
        auto kernel = Foundation::GaussianFunction::Produce1DKernel(20);
        std::move(kernel.begin(), kernel.end(), cbContent->Weights.begin());

        cbContent->InputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::PlaygroundRenderTarget);
        cbContent->OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::BlurResult);

        context->GetGraphicsDevice()->Dispatch(5, 720, 1);
    }

}
