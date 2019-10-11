#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/ResourceState.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceBarrier.hpp"

#include <functional>
#include <optional>

namespace PathFinder
{

    class PipelineResource
    {
    public:
        friend class PipelineResourceStorage;
        friend class ResoruceScheduler;

        struct PerPassEntities
        {
            std::optional<HAL::ResourceFormat::Color> ShaderVisibleFormat;
            bool IsRTDescriptorRequested = false;
            bool IsDSDescriptorRequested = false;
            bool IsSRDescriptorRequested = false;
            bool IsUADescriptorRequested = false;
        };

        std::optional<PerPassEntities> GetPerPassData(Foundation::Name passName) const;
        std::optional<HAL::ResourceTransitionBarrier> GetStateTransition() const;

    private:
        std::unique_ptr<HAL::Resource> mResource;
        std::unordered_map<Foundation::Name, PerPassEntities> mPerPassData;
        std::optional<HAL::ResourceTransitionBarrier> mOneTimeStateTransition;

    public:
        const HAL::Resource* Resource() const { return mResource.get(); }
    };

}
