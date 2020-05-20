#include "PipelineResourceSchedulingInfo.hpp"

namespace PathFinder
{

    PipelineResourceSchedulingInfo::PipelineResourceSchedulingInfo(Foundation::Name resourceName, const HAL::ResourceFormat& format, uint64_t resourceCount)
        : mResourceFormat{ format }, mResourceName{ resourceName }, mResourceCount{ resourceCount }, mSubresourceCount{ format.SubresourceCount() }
    {
        mResourceSchedulingMetadata.resize(resourceCount);

        for (SubresourceArray& innerArray : mResourceSchedulingMetadata)
        {
            innerArray.resize(mSubresourceCount);
        }
    }

    void PipelineResourceSchedulingInfo::FinishScheduling()
    {
        HAL::ResourceState initialStates = HAL::ResourceState::Common;
        HAL::ResourceState expectedStates = HAL::ResourceState::Common;

        // Get subresources for each resource in the array
        for (const SubresourceArray& subresources : mResourceSchedulingMetadata)
        {
            // Get per pass scheduling info for each subresource
            for (const PassInfoMap& subresourcePerPassData : subresources)
            {
                // Get scheduling metadata for each render pass that requested usage of this subresource
                for (const auto& [passName, metadata] : subresourcePerPassData)
                {
                    expectedStates |= metadata.RequestedState;

                    if (passName == mFirstPassGraphNode.PassMetadata.Name)
                    {
                        initialStates |= initialStates;
                    }
                }
            }
        }

        mInitialStates = initialStates;
        mExpectedStates = expectedStates;
        mResourceFormat.SetExpectedStates(expectedStates);
    }

    const PipelineResourceSchedulingInfo::PassInfo* PipelineResourceSchedulingInfo::GetInfoForPass(Foundation::Name passName, uint64_t resourceIndex, uint64_t subresourceIndex) const
    {
        const SubresourceArray& subresourceArray = mResourceSchedulingMetadata[resourceIndex];
        const PassInfoMap& passInfoMap = subresourceArray[subresourceIndex];
        auto it = passInfoMap.find(passName);
        return it != passInfoMap.end() ? &it->second : nullptr;
    }

    PipelineResourceSchedulingInfo::PassInfo* PipelineResourceSchedulingInfo::GetInfoForPass(Foundation::Name passName, uint64_t resourceIndex, uint64_t subresourceIndex)
    {
        SubresourceArray& subresourceArray = mResourceSchedulingMetadata[resourceIndex];
        PassInfoMap& passInfoMap = subresourceArray[subresourceIndex];
        auto it = passInfoMap.find(passName);
        return it != passInfoMap.end() ? &it->second : nullptr;
    }

    PipelineResourceSchedulingInfo::PassInfo& PipelineResourceSchedulingInfo::AllocateInfoForPass(const RenderPassExecutionGraph::Node& passNode, uint64_t resourceIndex, uint64_t subresourceIndex)
    {
        // Empty name means we have not set first pass node yet
        if (!mFirstPassGraphNode.PassMetadata.Name.IsValid())
        {
            mFirstPassGraphNode = passNode;
        }

        mLastPassGraphNode = passNode;

        auto [it, success] = mResourceSchedulingMetadata[resourceIndex][subresourceIndex].emplace(passNode.PassMetadata.Name, PassInfo{});
        return it->second;
    }

}
