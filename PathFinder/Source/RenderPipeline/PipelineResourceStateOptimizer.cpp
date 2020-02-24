#include "PipelineResourceStateOptimizer.hpp"
#include "RenderPass.hpp"

#include <limits>

namespace PathFinder
{

    PipelineResourceStateOptimizer::PipelineResourceStateOptimizer(const RenderPassExecutionGraph* renderPassGraph)
        : mRenderPassGraph{ renderPassGraph } {}

    void PipelineResourceStateOptimizer::AddSchedulingInfo(PipelineResourceSchedulingInfo* schedulingInfo)
    {
        mSchedulingInfos.push_back(schedulingInfo);
    }

    void PipelineResourceStateOptimizer::Optimize()
    {
        for (PipelineResourceSchedulingInfo* schedulingInfo : mSchedulingInfos)
        {
            CombineStateSequences(schedulingInfo);

            assert_format(!mCombinedStateSequences.empty(), "Resource mush have at least one state");

            for (auto i = 0; i < mCombinedStateSequences.size(); ++i)
            {
                auto& [passName, resourceState] = mCombinedStateSequences[i];
                schedulingInfo->GetMetadataForPass(passName)->OptimizedState = resourceState;
            }
        }
    }

    void PipelineResourceStateOptimizer::CombineStateSequences(PipelineResourceSchedulingInfo* schedulingInfo)
    {
        std::vector<Foundation::Name> relevantPassNames;
        std::vector<Foundation::Name> readOnlySequencePassNames;
        HAL::ResourceState readOnlyStateSequence = HAL::ResourceState::Common;

        // Build a list of passes this resource is scheduled for
        for (auto& passNode : mRenderPassGraph->AllPasses())
        {
            if (schedulingInfo->GetMetadataForPass(passNode.PassMetadata.Name))
            {
                relevantPassNames.push_back(passNode.PassMetadata.Name);
            }
        }

        if (relevantPassNames.empty())
        {
            return;
        }

        mCombinedStateSequences.clear();

        // Associate combined read states sequence with each corresponding pass name
        auto dumpReadOnlySequence = [this, &readOnlyStateSequence, &readOnlySequencePassNames]()
        {
            for (PassName passNameInReadOnlySequence : readOnlySequencePassNames)
            {
                mCombinedStateSequences.push_back({ passNameInReadOnlySequence, readOnlyStateSequence });
            }
            readOnlySequencePassNames.clear();
        };

        // Gather first state sequence
        for (auto relevantPassIdx = 0u; relevantPassIdx < relevantPassNames.size(); ++relevantPassIdx)
        {
            Foundation::Name currentPassName = relevantPassNames[relevantPassIdx];
            auto perPassData = schedulingInfo->GetMetadataForPass(currentPassName);

            bool isLastPass = relevantPassIdx == relevantPassNames.size() - 1;

            // Is state read-only
            if (HAL::IsResourceStateReadOnly(perPassData->RequestedState))
            {
                if (!HAL::IsResourceStateReadOnly(readOnlyStateSequence))
                {
                    // This is a first read-only state in a possible read-only sequence
                    readOnlySequencePassNames.push_back(currentPassName);
                    readOnlyStateSequence = perPassData->RequestedState;
                }
                else 
                {
                    // If previous state was read-only, then combine next with previous
                    readOnlyStateSequence |= perPassData->RequestedState;
                    readOnlySequencePassNames.push_back(currentPassName);
                }

                if (!isLastPass)
                {
                    // If next state is not read-only then this sequence should be dumped
                    PassName nextPassName = relevantPassNames[relevantPassIdx + 1];
                    auto nextPerPassData = schedulingInfo->GetMetadataForPass(nextPassName);

                    if (!HAL::IsResourceStateReadOnly(nextPerPassData->RequestedState))
                    {
                        dumpReadOnlySequence();
                    }
                }
                else
                {
                    // If there is no next state then this sequence definitely should be dumped
                    dumpReadOnlySequence();
                }
            }
            else 
            {
                // Write state is easy, just add it as a single-element sequence
                mCombinedStateSequences.push_back({ currentPassName, perPassData->RequestedState });
            }

            // Determine UAV barrier necessity
            if (EnumMaskBitSet(perPassData->RequestedState, HAL::ResourceState::UnorderedAccess))
            {
                perPassData->NeedsUAVBarrier = true;
            }
        }
    }

}

