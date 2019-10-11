#include "PipelineResource.hpp"

namespace PathFinder
{

    std::optional<PipelineResource::PerPassEntities> PipelineResource::GetPerPassData(Foundation::Name passName) const
    {
        auto it = mPerPassData.find(passName);
        if (it == mPerPassData.end()) return std::nullopt;
        return it->second;
    }

    std::optional<HAL::ResourceTransitionBarrier> PipelineResource::GetStateTransition() const
    {
        return mOneTimeStateTransition;
    }

}
