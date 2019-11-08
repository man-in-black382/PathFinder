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

        using StatePair = std::pair<HAL::ResourceState, HAL::ResourceState>;

        struct PassMetadata
        {
            HAL::ResourceState RequestedState;
            std::optional<HAL::ResourceFormat::Color> ShaderVisibleFormat;
            std::optional<StatePair> OptimizedTransitionStates;
            TextureRTDescriptorInserterPtr RTInserter = nullptr;
            TextureDSDescriptorInserterPtr DSInserter = nullptr;
            TextureSRDescriptorInserterPtr SRInserter = nullptr;
            TextureUADescriptorInserterPtr UAInserter = nullptr;
        };

        struct AliasingMetadata
        {
            const PipelineResourceAllocation* AliasingSource = nullptr;
            HAL::HeapAliasingGroup HeapAliasingGroup = HAL::HeapAliasingGroup::RTDSTextures;
            uint64_t HeapOffset = 0;
            bool NeedsAliasingBarrier = false;
        };

        PipelineResourceAllocation(const HAL::ResourceFormat& format);

        void GatherExpectedStates();
        const PassMetadata* GetMetadataForPass(Foundation::Name passName) const;
        PassMetadata* GetMetadataForPass(Foundation::Name passName);
        PassMetadata& AllocateMetadataForPass(Foundation::Name passName);

        std::function<void()> AllocationAction;
        std::optional<StatePair> OneTimeTransitionStates;
        AliasingMetadata AliasingInfo;

    private:
        std::unordered_map<Foundation::Name, PassMetadata> mPerPassData;
        Foundation::Name mFirstPassName;
        Foundation::Name mLastPassName;
        HAL::ResourceFormat mResourceFormat;
        HAL::ResourceState mExpectedStates;

    public:
        inline const auto& AllPassesMetadata() const { return mPerPassData; }
        inline Foundation::Name FirstPassName() const { return mFirstPassName; }
        inline Foundation::Name LastPassName() const { return mLastPassName; }
        inline const HAL::ResourceFormat& ResourceFormat() const { return mResourceFormat; }
        inline HAL::ResourceState ExpectedStates() const { return mExpectedStates; }
    };

}
