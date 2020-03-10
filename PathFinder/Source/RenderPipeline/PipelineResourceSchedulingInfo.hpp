#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/ResourceState.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include "RenderPassExecutionGraph.hpp"

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

        PipelineResourceSchedulingInfo(const HAL::ResourceFormat& format, Foundation::Name resourceName);

        void FinishScheduling();
        const PassMetadata* GetMetadataForPass(Foundation::Name passName) const;
        PassMetadata* GetMetadataForPass(Foundation::Name passName);
        PassMetadata& AllocateMetadataForPass(const RenderPassExecutionGraph::Node& passNode);
        HAL::ResourceState InitialStates() const;

        std::function<void()> AllocationAction;
        AliasingMetadata AliasingInfo;

    private:
        std::unordered_map<Foundation::Name, PassMetadata> mPerPassData;
        RenderPassExecutionGraph::Node mFirstPassGraphNode;
        RenderPassExecutionGraph::Node mLastPassGraphNode;
        HAL::ResourceFormat mResourceFormat;
        HAL::ResourceState mExpectedStates = HAL::ResourceState::Common;
        Foundation::Name mResourceName;

    public:
        inline const auto& AllPassesMetadata() const { return mPerPassData; }
        inline const RenderPassExecutionGraph::Node& FirstPassGraphNode() const { return mFirstPassGraphNode; }
        inline const RenderPassExecutionGraph::Node& LastPassGraphNode() const { return mLastPassGraphNode; }
        inline const HAL::ResourceFormat& ResourceFormat() const { return mResourceFormat; }
        inline HAL::ResourceState ExpectedStates() const { return mExpectedStates; }
        inline Foundation::Name ResourceName() const { return mResourceName; }
    };

}
