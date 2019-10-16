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

        //scheduler->NewTexture("3", ResourceScheduler::NewTextureProperties{ std::nullopt,  Geometry::Dimensions{300, 100}, std::nullopt, std::nullopt });
        //scheduler->NewTexture("4", ResourceScheduler::NewTextureProperties{ std::nullopt,  Geometry::Dimensions{400, 100}, std::nullopt, std::nullopt });
    }
     
    void BlurRenderPass::Render(RenderContext* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::Blur);

        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<BlurCBContent>();
        cbContent->BlurRadius = 20;
        auto kernel = Foundation::GaussianFunction::Produce1DKernel(20);
        std::move(kernel.begin(), kernel.end(), cbContent->Weights.begin());

        cbContent->InputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::PlaygroundRenderTarget);
        cbContent->OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::BlurResult);

        context->GetCommandRecorder()->Dispatch(5, 720, 1);
    }

}
