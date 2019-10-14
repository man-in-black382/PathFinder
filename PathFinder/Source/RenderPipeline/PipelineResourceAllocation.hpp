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

        PipelineResourceAllocation(const HAL::ResourceFormat& format);

        HAL::ResourceState GatherExpectedStates() const;
        const PerPassEntities* GetMetadataForPass(Foundation::Name passName) const;
        PerPassEntities& AllocateMetadataForPass(Foundation::Name passName);

        HAL::ResourceFormat Format;
        std::function<void()> AllocationAction;
        uint64_t HeapOffset = 0;

    private:
        std::unordered_map<Foundation::Name, PerPassEntities> mPerPassData;
        Foundation::Name mFirstPassName;
        Foundation::Name mLastPassName;

    public:
        inline const auto& AllPassesMetadata() const { return mPerPassData; }
        inline Foundation::Name FirstPassName() const { return mFirstPassName; }
        inline Foundation::Name LastPassName() const { return mLastPassName; }
    };

}
