#include "PipelineResourceSchedulingInfo.hpp"

namespace PathFinder
{

    PipelineResourceSchedulingInfo::PipelineResourceSchedulingInfo(Foundation::Name resourceName, const HAL::ResourceFormat& format, uint64_t resourceCount)
        : mResourceFormat{ format }, mResourceName{resourceName}, mResourceCount{ resourceCount }
    {
        mAllResourcesPerPassData.resize(resourceCount);
    }

    void PipelineResourceSchedulingInfo::FinishScheduling()
    {
        HAL::ResourceState initialStates = HAL::ResourceState::Common;
        HAL::ResourceState expectedStates = HAL::ResourceState::Common;

        for (auto& resourcePerPassData : mAllResourcesPerPassData)
        {
            for (const auto& [passName, metadata] : resourcePerPassData)
            {
                expectedStates |= metadata.RequestedState;

                if (passName == mFirstPassGraphNode.PassMetadata.Name)
                {
                    initialStates |= initialStates;
                }
            }
        }

        mInitialStates = initialStates;
        mExpectedStates = expectedStates;
        mResourceFormat.SetExpectedStates(expectedStates);
    }

    const PipelineResourceSchedulingInfo::PassMetadata* PipelineResourceSchedulingInfo::GetMetadataForPass(Foundation::Name passName, uint64_t resourceIndex) const
    {
        auto& resourcePerPassData = mAllResourcesPerPassData[resourceIndex];
        auto it = resourcePerPassData.find(passName);
        return it != resourcePerPassData.end() ? &it->second : nullptr;
    }

    PipelineResourceSchedulingInfo::PassMetadata* PipelineResourceSchedulingInfo::GetMetadataForPass(Foundation::Name passName, uint64_t resourceIndex)
    {
        auto& resourcePerPassData = mAllResourcesPerPassData[resourceIndex];
        auto it = resourcePerPassData.find(passName);
        return it != resourcePerPassData.end() ? &it->second : nullptr;
    }

    PipelineResourceSchedulingInfo::PassMetadata& PipelineResourceSchedulingInfo::AllocateMetadataForPass(const RenderPassExecutionGraph::Node& passNode, uint64_t resourceIndex)
    {
        // Empty name means we have not set first pass node yet
        if (!mFirstPassGraphNode.PassMetadata.Name.IsValid())
        {
            mFirstPassGraphNode = passNode;
        }

        mLastPassGraphNode = passNode;

        auto [it, success] = mAllResourcesPerPassData[resourceIndex].emplace(passNode.PassMetadata.Name, PassMetadata{});
        return it->second;
    }

}
