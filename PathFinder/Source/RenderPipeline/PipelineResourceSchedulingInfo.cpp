#include "PipelineResourceSchedulingInfo.hpp"

namespace PathFinder
{

    PipelineResourceSchedulingInfo::PipelineResourceSchedulingInfo(const HAL::ResourceFormat& format, Foundation::Name resourceName, uint32_t resourceCount)
        : mResourceFormat{ format }, mResourceName{ resourceName }, mResourceCount{ resourceCount } {}

    void PipelineResourceSchedulingInfo::FinishScheduling()
    {
        HAL::ResourceState expectedStates = HAL::ResourceState::Common;

        for (const auto& pair : mPerPassData)
        {
            const PassMetadata& perPassData = pair.second;
            expectedStates |= perPassData.RequestedState;
        }

        mExpectedStates = expectedStates;

        mResourceFormat.SetExpectedStates(expectedStates);
    }

    const PipelineResourceSchedulingInfo::PassMetadata* PipelineResourceSchedulingInfo::GetMetadataForPass(Foundation::Name passName) const
    {
        auto it = mPerPassData.find(passName);
        return it != mPerPassData.end() ? &it->second : nullptr;
    }

    PipelineResourceSchedulingInfo::PassMetadata* PipelineResourceSchedulingInfo::GetMetadataForPass(Foundation::Name passName)
    {
        auto it = mPerPassData.find(passName);
        return it != mPerPassData.end() ? &it->second : nullptr;
    }

    PipelineResourceSchedulingInfo::PassMetadata& PipelineResourceSchedulingInfo::AllocateMetadataForPass(const RenderPassExecutionGraph::Node& passNode)
    {
        // Empty name means we have not set first pass node yet
        if (!mFirstPassGraphNode.PassMetadata.Name.IsValid())
        {
            mFirstPassGraphNode = passNode;
        }

        mLastPassGraphNode = passNode;

        auto [iter, success] = mPerPassData.emplace(passNode.PassMetadata.Name, PassMetadata{});
        return iter->second;
    }

    HAL::ResourceState PipelineResourceSchedulingInfo::InitialStates() const
    {
        auto firstPassMetadata = GetMetadataForPass(mFirstPassGraphNode.PassMetadata.Name);
        return firstPassMetadata->OptimizedState;
    }

}
