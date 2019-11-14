#include "PipelineResourceStateOptimizer.hpp"
#include "RenderPass.hpp"

#include <limits>

namespace PathFinder
{

    PipelineResourceStateOptimizer::PipelineResourceStateOptimizer(const RenderPassExecutionGraph* renderPassGraph)
        : mRenderPassGraph{ renderPassGraph } {}

    void PipelineResourceStateOptimizer::AddAllocation(PipelineResourceAllocation* allocation)
    {
        mAllocations.push_back(allocation);
    }

    void PipelineResourceStateOptimizer::Optimize()
    {
        for (PipelineResourceAllocation* allocation : mAllocations)
        {
            CollapseStateSequences(allocation);

            assert_format(!mCollapsedStateSequences.empty(), "Resource mush have at least one state");

            // If resource will have only one state then it's sufficient 
            // to make a single transition in the first rendering loop
            //
            if (mCollapsedStateSequences.size() == 1)
            {
                HAL::ResourceState nextState = mCollapsedStateSequences.back().second;
                allocation->OneAndOnlyState = nextState;
                continue;
            }

            // See whether automatic state transitions are possible
            bool canImplicitlyPromoteToFirstState = allocation->ResourceFormat().CanResourceImplicitlyPromoteFromCommonStateToState(mCollapsedStateSequences.front().second);
            bool canImplicitlyDecayFromLastState = allocation->ResourceFormat().CanResourceImplicitlyDecayToCommonStateFromState(mCollapsedStateSequences.back().second);
            bool canSkipFirstTransition = canImplicitlyPromoteToFirstState && canImplicitlyDecayFromLastState;

            uint32_t firstExplicitTransitionIndex = 0;

            if (canSkipFirstTransition)
            {
                // Skip first transition if possible
                //
                PassName secondPassName = mCollapsedStateSequences[1].first;
                HAL::ResourceState implicitFirstState = mCollapsedStateSequences[0].second;
                HAL::ResourceState explicitSecondState = mCollapsedStateSequences[1].second;

                allocation->GetMetadataForPass(secondPassName)->OptimizedTransitionStates = std::make_pair(implicitFirstState, explicitSecondState);

                firstExplicitTransitionIndex = 1;
            }
            else {
                // No way to skip first transition, loop last transition to first
                //
                PassName firstPassName = mCollapsedStateSequences[0].first;
                HAL::ResourceState explicitLastState = mCollapsedStateSequences[mCollapsedStateSequences.size() - 1].second;
                HAL::ResourceState explicitFirstState = mCollapsedStateSequences[0].second;

                allocation->GetMetadataForPass(firstPassName)->OptimizedTransitionStates = std::make_pair(explicitLastState, explicitFirstState);
            }

            // Create the rest of the transitions
            for (auto i = firstExplicitTransitionIndex + 1; i < mCollapsedStateSequences.size(); ++i)
            {
                PassName currentPassName = mCollapsedStateSequences[i].first;
                allocation->GetMetadataForPass(currentPassName)->OptimizedTransitionStates = 
                    std::make_pair(mCollapsedStateSequences[i - 1].second, mCollapsedStateSequences[i].second);
            }
        }
    }

    void PipelineResourceStateOptimizer::CollapseStateSequences(const PipelineResourceAllocation* allocation)
    {
        std::vector<Foundation::Name> relevantPassNames;
        HAL::ResourceState stateSequence = HAL::ResourceState::Common;
        Foundation::Name stateTransitionPassName;

        // Build a list of passes this resource is scheduled for
        for (const RenderPass* pass : mRenderPassGraph->DefaultPasses())
        {
            if (allocation->GetMetadataForPass(pass->Name()))
            {
                relevantPassNames.push_back(pass->Name());
            }
        }

        if (relevantPassNames.empty())
        {
            return;
        }

        mCollapsedStateSequences.clear();

        // Gather first state sequence
        for (auto relevantPassIdx = 0u; relevantPassIdx < relevantPassNames.size(); ++relevantPassIdx)
        {
            Foundation::Name currentPassName = relevantPassNames[relevantPassIdx];
            auto perPassData = allocation->GetMetadataForPass(currentPassName);

            bool isLastPass = relevantPassIdx == relevantPassNames.size() - 1;

            // Is state read-only
            if (HAL::IsResourceStateReadOnly(perPassData->RequestedState))
            {
                if (HAL::IsResourceStateReadOnly(stateSequence))
                {
                    // If previous state was read-only, then combine next with previous
                    stateSequence |= perPassData->RequestedState;
                }
                else {
                    // This is a first read-only state in a possible read-only sequence
                    stateSequence = perPassData->RequestedState;
                    stateTransitionPassName = currentPassName;
                }

                if (!isLastPass)
                {
                    // If next state is not read-only then this sequence should be dumped
                    PassName nextPassName = relevantPassNames[relevantPassIdx + 1];
                    auto nextPerPassData = allocation->GetMetadataForPass(nextPassName);

                    if (!HAL::IsResourceStateReadOnly(nextPerPassData->RequestedState))
                    {
                        mCollapsedStateSequences.push_back({ stateTransitionPassName, stateSequence });
                    }
                }
                else {
                    // If there is no next state then this sequence definitely should be dumped
                    mCollapsedStateSequences.push_back({ stateTransitionPassName, stateSequence });
                }
            }
            else {
                stateSequence = perPassData->RequestedState;
                stateTransitionPassName = currentPassName;

                mCollapsedStateSequences.push_back({ stateTransitionPassName, stateSequence });
            }
        }
    }

}

