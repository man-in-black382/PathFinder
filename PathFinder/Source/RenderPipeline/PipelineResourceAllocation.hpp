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
        using StatePair = std::pair<HAL::ResourceState, HAL::ResourceState>;

        struct PassMetadata
        {
            HAL::ResourceState RequestedState = HAL::ResourceState::Common;
            std::optional<HAL::ResourceFormat::Color> ShaderVisibleFormat;
            std::optional<StatePair> OptimizedTransitionStates;
            bool CreateTextureRTDescriptor = false;
            bool CreateTextureDSDescriptor = false;
            bool CreateTextureSRDescriptor = false;
            bool CreateTextureUADescriptor = false;
            bool CreateBufferCBDescriptor = false;
            bool CreateBufferSRDescriptor = false;
            bool CreateBufferUADescriptor = false;
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
        HAL::ResourceState InitialStates() const;

        std::function<void()> AllocationAction;
        std::optional<HAL::ResourceState> OneAndOnlyState;
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
