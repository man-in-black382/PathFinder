#include "PipelineResourceAllocation.hpp"

namespace PathFinder
{

    PipelineResourceAllocation::PipelineResourceAllocation(const HAL::ResourceFormat& format)
        : Format{ format } {}

    HAL::ResourceState PipelineResourceAllocation::GatherExpectedStates() const
    {
        HAL::ResourceState expectedStates = HAL::ResourceState::Common;

        for (const auto& pair : mPerPassData)
        {
            const PerPassEntities& perPassData = pair.second;
            expectedStates |= perPassData.RequestedState;
        }

        return expectedStates;
    }

    const PipelineResourceAllocation::PerPassEntities* PipelineResourceAllocation::GetMetadataForPass(Foundation::Name passName) const
    {
        auto it = mPerPassData.find(passName);
        return it != mPerPassData.end() ? &it->second : nullptr;
    }

    PipelineResourceAllocation::PerPassEntities& PipelineResourceAllocation::AllocateMetadataForPass(Foundation::Name passName)
    {
        if (!mFirstPassName.IsValid())
        {
            mFirstPassName = passName;
        }

        mLastPassName = passName;

        auto [iter, success] = mPerPassData.emplace(passName, PerPassEntities{});
        return iter->second;
    }

}
