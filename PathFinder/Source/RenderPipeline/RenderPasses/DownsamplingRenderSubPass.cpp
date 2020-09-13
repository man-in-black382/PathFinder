#include "DownsamplingRenderSubPass.hpp"

namespace PathFinder
{

    DownsamplingRenderSubPass::DownsamplingRenderSubPass(const std::string& name, uint64_t invocationIndex)
        : RenderSubPass(CombineNameAndInvocation(name, invocationIndex)) {}

    DownsamplingRenderSubPass::DownsamplingRenderSubPass(const std::string& name)
        : RenderSubPass(name) {}

    void DownsamplingRenderSubPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        for (const DownsamplingInvocationInputs& inputs : mInvocationInputs)
        {
            scheduler->ReadTexture(inputs.ResourceName, ResourceScheduler::MipSet::IndexFromStart(inputs.SourceMip));
            
            for (uint8_t outputIdx = 0; outputIdx < DownsamplingInvocationInputs::MaxMipsProcessedByInvocation; ++outputIdx)
            {
                if (inputs.CBContent.OutputsToWrite[outputIdx])
                {
                    scheduler->WriteTexture(inputs.ResourceName, ResourceScheduler::MipSet::IndexFromStart(inputs.SourceMip + outputIdx + 1));
                }
            }
        }
    }

    void DownsamplingRenderSubPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::Downsampling);

        for (DownsamplingInvocationInputs& inputs : mInvocationInputs)
        {
            UpdateDownsamplingInputsWithTextureIndices(inputs, context->GetResourceProvider());
            context->GetConstantsUpdater()->UpdateRootConstantBuffer(inputs.CBContent);
            context->GetCommandRecorder()->Dispatch(inputs.DispatchDimensions, { 8, 8 });
        }
    }

    void DownsamplingRenderSubPass::SetInvocationInputs(const std::vector<DownsamplingInvocationInputs>& inputs)
    {
        mInvocationInputs = inputs;
    }

    Foundation::Name DownsamplingRenderSubPass::CombineNameAndInvocation(const std::string& name, uint64_t invocationIndex) const
    {
        return { name + "_Invocation[" + std::to_string(invocationIndex) + "]" };
    }

}
