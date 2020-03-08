#include "BloomRenderPass.hpp"

namespace PathFinder
{

    BloomRenderPass::BloomRenderPass()
        : RenderPass("Bloom") {}

    void BloomRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::GaussianBlur, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "BloomDownscaling.hlsl";
        });
    }

    void BloomRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->ReadWriteTexture(ResourceNames::DeferredLightingOversaturatedOutput);
        scheduler->NewTexture(ResourceNames::BloomFullResolution);
    }
     
    void BloomRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::GaussianBlur);

        /*BlurCBContent cbContent{};
        cbContent.BlurRadius = 20;
        auto kernel = Foundation::GaussianFunction::Produce1DKernel(20);
        std::move(kernel.begin(), kernel.end(), cbContent.Weights.begin());

        cbContent.InputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferRT0);
        cbContent.OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::BloomFullResolution);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(5, 720, 1);*/
    }

}
