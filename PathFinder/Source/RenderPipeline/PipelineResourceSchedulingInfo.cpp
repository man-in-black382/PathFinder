#include "PipelineResourceSchedulingInfo.hpp"

namespace PathFinder
{

    PipelineResourceSchedulingInfo::PipelineResourceSchedulingInfo(const HAL::ResourceFormat& format)
        : mResourceFormat{ format } {}

    void PipelineResourceSchedulingInfo::GatherExpectedStates()
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

    PipelineResourceSchedulingInfo::PassMetadata& PipelineResourceSchedulingInfo::AllocateMetadataForPass(Foundation::Name passName)
    {
        if (!mFirstPassName.IsValid())
        {
            mFirstPassName = passName;
        }

        mLastPassName = passName;

        auto [iter, success] = mPerPassData.emplace(passName, PassMetadata{});
        return iter->second;
    }

    HAL::ResourceState PipelineResourceSchedulingInfo::InitialStates() const
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
