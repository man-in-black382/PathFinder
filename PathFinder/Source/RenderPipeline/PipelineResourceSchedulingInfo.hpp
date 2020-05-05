#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/ResourceState.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include "RenderPassExecutionGraph.hpp"

#include <functional>
#include <optional>

namespace PathFinder
{

    /// A helper class to hold all info necessary for resource allocation.
    /// Filled by resource scheduling infrastructure.
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

        using MetadataIterator = std::function<void(PassMetadata&)>;

        // An array of GPU resources can be scheduled and allocated per one resource name.
        // Each resource in the array has it's own set of scheduling metadata.
        using AllResourcePerPassMetadata = std::vector<std::unordered_map<Foundation::Name, PassMetadata>>;

        struct AliasingMetadata
        {
            const PipelineResourceSchedulingInfo* AliasingSource = nullptr;
            uint64_t HeapOffset = 0;
            bool NeedsAliasingBarrier = false;
        };

        PipelineResourceSchedulingInfo(const HAL::ResourceFormat& format, uint64_t resourceCount);

        void FinishScheduling();
        const PassMetadata* GetMetadataForPass(Foundation::Name passName, uint64_t resourceIndex) const;
        PassMetadata* GetMetadataForPass(Foundation::Name passName, uint64_t resourceIndex);
        PassMetadata& AllocateMetadataForPass(const RenderPassExecutionGraph::Node& passNode, uint64_t resourceIndex);

        std::function<void()> AllocationAction;
        AliasingMetadata AliasingInfo;

    private:
        // Per pass data per each resource in resource array
        AllResourcePerPassMetadata mAllResourcesPerPassData;
        RenderPassExecutionGraph::Node mFirstPassGraphNode;
        RenderPassExecutionGraph::Node mLastPassGraphNode;
        HAL::ResourceFormat mResourceFormat;
        HAL::ResourceState mInitialStates = HAL::ResourceState::Common;
        HAL::ResourceState mExpectedStates = HAL::ResourceState::Common;
        uint64_t mResourceCount = 0;

    public:
        inline const auto& AllPassesMetadata() const { return mAllResourcesPerPassData; }
        inline const RenderPassExecutionGraph::Node& FirstPassGraphNode() const { return mFirstPassGraphNode; }
        inline const RenderPassExecutionGraph::Node& LastPassGraphNode() const { return mLastPassGraphNode; }
        inline const HAL::ResourceFormat& ResourceFormat() const { return mResourceFormat; }
        inline HAL::ResourceState InitialStates() const { return mInitialStates; }
        inline HAL::ResourceState ExpectedStates() const { return mExpectedStates; }
        inline auto ResourceCount() const { return mResourceCount; }
        inline auto TotalRequiredMemory() const { return mResourceFormat.ResourceSizeInBytes() * mResourceCount; }
    };

}
