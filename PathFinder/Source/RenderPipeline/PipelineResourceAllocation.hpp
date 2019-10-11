#pragma once

#include "ResourceDescriptorStorage.hpp"

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/ResourceState.hpp"

#include <functional>
#include <optional>

namespace PathFinder
{

    class PipelineResourceAllocation
    {
    public:
        friend class PipelineResourceStorage;
        friend class ResourceScheduler;

        using TextureRTDescriptorInserterPtr = decltype(&ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded);
        using TextureDSDescriptorInserterPtr = decltype(&ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded);
        using TextureSRDescriptorInserterPtr = decltype(&ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded);
        using TextureUADescriptorInserterPtr = decltype(&ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded);

        struct PerPassEntities
        {
            HAL::ResourceState RequestedState;
            std::optional<HAL::ResourceFormat::Color> ShaderVisibleFormat;
            TextureRTDescriptorInserterPtr RTInserter = nullptr;
            TextureDSDescriptorInserterPtr DSInserter = nullptr;
            TextureSRDescriptorInserterPtr SRInserter = nullptr;
            TextureUADescriptorInserterPtr UAInserter = nullptr;
        };

        HAL::ResourceState GatherExpectedStates() const;
        std::optional<PerPassEntities> GetPerPassData(Foundation::Name passName) const;

    private:
        HAL::ResourceFormat::FormatVariant mFormat;
        std::function<void()> mAllocationAction;
        std::unordered_map<Foundation::Name, PerPassEntities> mPerPassData;
    };

}
