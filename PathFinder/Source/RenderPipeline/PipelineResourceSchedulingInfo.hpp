#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/ResourceState.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include <functional>
#include <optional>

namespace PathFinder
{

    class PipelineResourceSchedulingInfo
    {
    public:
        struct PassMetadata
        {
            HAL::ResourceState RequestedState = HAL::ResourceState::Common;
            std::optional<HAL::ColorFormat> ShaderVisibleFormat;
            HAL::ResourceState OptimizedState = HAL::ResourceState::Common;
            bool NeedsUAVBarrier = false;
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
            const PipelineResourceSchedulingInfo* AliasingSource = nullptr;
            uint64_t HeapOffset = 0;
            bool NeedsAliasingBarrier = false;
        };

        PipelineResourceSchedulingInfo(const HAL::ResourceFormat& format);

        void GatherExpectedStates();
        const PassMetadata* GetMetadataForPass(Foundation::Name passName) const;
        PassMetadata* GetMetadataForPass(Foundation::Name passName);
        PassMetadata& AllocateMetadataForPass(Foundation::Name passName);
        HAL::ResourceState InitialStates() const;

        std::function<void()> AllocationAction;
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
