#include "PipelineResourceAllocation.hpp"

namespace PathFinder
{

    PipelineResourceAllocation::PipelineResourceAllocation(const HAL::ResourceFormat& format)
        : mResourceFormat{ format } {}

    void PipelineResourceAllocation::GatherExpectedStates()
    {
        HAL::ResourceState expectedStates = HAL::ResourceState::Common;

        for (const auto& pair : mPerPassData)
        {
            const PassMetadata& perPassData = pair.second;
            expectedStates |= perPassData.RequestedState;
        }

        mExpectedStates = expectedStates;

        mResourceFormat.UpdateExpectedUsageFlags(expectedStates);
    }

    const PipelineResourceAllocation::PassMetadata* PipelineResourceAllocation::GetMetadataForPass(Foundation::Name passName) const
    {
        auto it = mPerPassData.find(passName);
        return it != mPerPassData.end() ? &it->second : nullptr;
    }

    PipelineResourceAllocation::PassMetadata* PipelineResourceAllocation::GetMetadataForPass(Foundation::Name passName)
    {
        auto it = mPerPassData.find(passName);
        return it != mPerPassData.end() ? &it->second : nullptr;
    }

    PipelineResourceAllocation::PassMetadata& PipelineResourceAllocation::AllocateMetadataForPass(Foundation::Name passName)
    {
        if (!mFirstPassName.IsValid())
        {
            mFirstPassName = passName;
        }

        mLastPassName = passName;

        auto [iter, success] = mPerPassData.emplace(passName, PassMetadata{});
        return iter->second;
    }

    HAL::ResourceState PipelineResourceAllocation::InitialStates() const
    {
        if (OneAndOnlyState) return *OneAndOnlyState;

        auto firstPassMetadata = GetMetadataForPass(mFirstPassName);

        if (firstPassMetadata && firstPassMetadata->OptimizedTransitionStates)
        {
            return firstPassMetadata->OptimizedTransitionStates->first;
        } 
        else {
            return HAL::ResourceState::Common;
        }
    }

}
