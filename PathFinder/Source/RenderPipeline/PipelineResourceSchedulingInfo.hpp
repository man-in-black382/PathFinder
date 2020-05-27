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
        class PassInfo
        {
        public:
            using RequestedAccessFlag = uint16_t;

            HAL::ResourceState RequestedState = HAL::ResourceState::Common;
            std::optional<HAL::ColorFormat> ShaderVisibleFormat;
            HAL::ResourceState OptimizedState = HAL::ResourceState::Common;
            bool NeedsUAVBarrier = false;

            void SetTextureRTRequested() { mAccessFlag = 1 << 0; }
            void SetTextureDSRequested() { mAccessFlag = 1 << 1; }
            void SetTextureSRRequested() { mAccessFlag = 1 << 2; }
            void SetTextureUARequested() { mAccessFlag = 1 << 3; }
            void SetBufferCBRequested() { mAccessFlag = 1 << 4; }
            void SetBufferSRRequested() { mAccessFlag = 1 << 5; }
            void SetBufferUARequested() { mAccessFlag = 1 << 6; }

            bool IsTextureRTRequested() const { return mAccessFlag & 1 << 0; }
            bool IsTextureDSRequested() const { return mAccessFlag & 1 << 1; }
            bool IsTextureSRRequested() const { return mAccessFlag & 1 << 2; }
            bool IsTextureUARequested() const { return mAccessFlag & 1 << 3; }
            bool IsBufferCBRequested() const { return mAccessFlag & 1 << 4; }
            bool IsBufferSRRequested() const { return mAccessFlag & 1 << 5; }
            bool IsBufferUARequested() const { return mAccessFlag & 1 << 6; }

            bool IsWriteAccessRequested() const {
                return IsTextureRTRequested() || IsTextureDSRequested() || IsTextureUARequested() || IsBufferUARequested();
            }

        private:
            RequestedAccessFlag mAccessFlag;
        };

        using PassInfoIterator = std::function<void(PassInfo&)>;

        // An array of GPU resources can be scheduled and allocated per one resource name.
        // Each resource in the array has it's own array of scheduling metadata per each sub resource.
        using PassInfoMap = std::unordered_map<Foundation::Name, PassInfo>;
        using SubresourceArray = std::vector<PassInfoMap>;
        using ResourceArray = std::vector<SubresourceArray>;

        struct AliasingInfo
        {
            uint64_t HeapOffset = 0;
            bool NeedsAliasingBarrier = false;
            bool IsAliased = false;
        };

        PipelineResourceSchedulingInfo(Foundation::Name resourceName, const HAL::ResourceFormat& format, uint64_t resourceCount);

        void FinishScheduling();
        const PassInfo* GetInfoForPass(Foundation::Name passName, uint64_t resourceIndex, uint64_t subresourceIndex) const;
        PassInfo* GetInfoForPass(Foundation::Name passName, uint64_t resourceIndex, uint64_t subresourceIndex);
        PassInfo& AllocateInfoForPass(const RenderPassExecutionGraph::Node& passNode, uint64_t resourceIndex, uint64_t subresourceIndex);

        std::function<void()> AllocationAction;
        AliasingInfo MemoryAliasingInfo;

    private:
        ResourceArray mResourceSchedulingMetadata;
        RenderPassExecutionGraph::Node mFirstPassGraphNode;
        RenderPassExecutionGraph::Node mLastPassGraphNode;
        HAL::ResourceFormat mResourceFormat;
        HAL::ResourceState mInitialStates = HAL::ResourceState::Common;
        HAL::ResourceState mExpectedStates = HAL::ResourceState::Common;
        Foundation::Name mResourceName;
        uint64_t mResourceCount = 0;
        uint64_t mSubresourceCount = 0;

    public:
        inline const auto& AllPassesMetadata() const { return mResourceSchedulingMetadata; }
        inline const RenderPassExecutionGraph::Node& FirstPassGraphNode() const { return mFirstPassGraphNode; }
        inline const RenderPassExecutionGraph::Node& LastPassGraphNode() const { return mLastPassGraphNode; }
        inline const HAL::ResourceFormat& ResourceFormat() const { return mResourceFormat; }
        inline HAL::ResourceState InitialStates() const { return mInitialStates; }
        inline HAL::ResourceState ExpectedStates() const { return mExpectedStates; }
        inline Foundation::Name ResourceName() const { return mResourceName; }
        inline auto ResourceCount() const { return mResourceCount; }
        inline auto SubresourceCount() const { return mSubresourceCount; }
        inline auto TotalRequiredMemory() const { return mResourceFormat.ResourceSizeInBytes() * mResourceCount; }
    };

}
