#include "DenoiserMipGenerationRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserMipGenerationRenderPass::DenoiserMipGenerationRenderPass()
        : RenderPass("DenoiserMipGeneration") {}

    void DenoiserMipGenerationRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
    }

    void DenoiserMipGenerationRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto frameIndex = scheduler->FrameNumber() % 2;

        scheduler->ReadTexture(ResourceNames::GBufferViewDepth[frameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingPreBlurred, ResourceScheduler::MipSet::FirstMip());
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred, ResourceScheduler::MipSet::FirstMip());
        scheduler->ReadTexture(ResourceNames::StochasticShadingGradientNormFactor, ResourceScheduler::MipSet::FirstMip());

        scheduler->WriteTexture(ResourceNames::GBufferViewDepth[frameIndex], ResourceScheduler::MipSet::Range(1, std::nullopt));
        scheduler->WriteTexture(ResourceNames::StochasticShadowedShadingPreBlurred, ResourceScheduler::MipSet::Range(1, std::nullopt));
        scheduler->WriteTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred, ResourceScheduler::MipSet::Range(1, std::nullopt));
        scheduler->WriteTexture(ResourceNames::StochasticShadingGradientNormFactor, ResourceScheduler::MipSet::Range(1, std::nullopt));
    }
     
    void DenoiserMipGenerationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::Downsampling);

        auto resourceProvider = context->GetResourceProvider();
        auto frameIndex = context->FrameNumber() % 2;

        GenerateMips(ResourceNames::GBufferViewDepth[frameIndex], context, DownsamplingCBContent::Filter::Min, DownsamplingStrategy::WriteAllLevels);
        GenerateMips(ResourceNames::StochasticUnshadowedShadingPreBlurred, context, DownsamplingCBContent::Filter::Average, DownsamplingStrategy::WriteAllLevels);
        GenerateMips(ResourceNames::StochasticShadowedShadingPreBlurred, context, DownsamplingCBContent::Filter::Average, DownsamplingStrategy::WriteAllLevels);
        GenerateMips(ResourceNames::StochasticShadingGradientNormFactor, context, DownsamplingCBContent::Filter::Max, DownsamplingStrategy::WriteOnlyLastLevel);
    }

    void DenoiserMipGenerationRenderPass::GenerateMips(Foundation::Name resourceName, RenderContext<RenderPassContentMediator>* context, DownsamplingCBContent::Filter filter, DownsamplingStrategy strategy)
    {
        auto inputs = GenerateDownsamplingShaderInvocationInputs(*context->GetResourceProvider(), resourceName, filter, strategy);

        for (const DownsamplingInvocationInputs& inputs : inputs)
        {
            context->GetConstantsUpdater()->UpdateRootConstantBuffer(inputs.CBContent);
            context->GetCommandRecorder()->Dispatch(inputs.DispatchDimensions, { 8, 8 });
        }
    }

}
