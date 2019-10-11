#include "PipelineResourceAllocation.hpp"

namespace PathFinder
{

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

    std::optional<PipelineResourceAllocation::PerPassEntities> PipelineResourceAllocation::GetPerPassData(Foundation::Name passName) const
    {
        auto it = mPerPassData.find(passName);
        if (it == mPerPassData.end()) return std::nullopt;
        return it->second;
    }

}
