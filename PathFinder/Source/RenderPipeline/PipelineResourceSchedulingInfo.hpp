#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/ResourceState.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include "RenderPassGraph.hpp"

#include <functional>
#include <optional>

namespace PathFinder
{

    /// A helper class to hold all info necessary for resource allocation.
    /// Filled by resource scheduling infrastructure.
    class PipelineResourceSchedulingInfo
    {
    public:
        class SubresourceInfo
        {
        public:
            enum class AccessFlag
            {
                TextureRT, TextureDS, TextureSR, TextureUA, BufferSR, BufferUA, BufferCB
            };

            HAL::ResourceState RequestedState = HAL::ResourceState::Common;
            std::optional<HAL::ColorFormat> ShaderVisibleFormat;
            AccessFlag AccessValidationFlag;
        };

        struct PassInfo
        {
            std::vector<std::optional<SubresourceInfo>> SubresourceInfos;
            bool NeedsUnorderedAccessBarrier = false;
            bool NeedsAliasingBarrier = false;
        };

        PipelineResourceSchedulingInfo(Foundation::Name resourceName, const HAL::ResourceFormat& format);

        void AddExpectedStates(HAL::ResourceState states);
        void AddNameAlias(Foundation::Name alias);
        void ApplyExpectedStates();
        const PassInfo* GetInfoForPass(Foundation::Name passName) const;
        PassInfo* GetInfoForPass(Foundation::Name passName);

        void SetSubresourceInfo(
            Foundation::Name passName, 
            uint64_t subresourceIndex, 
            HAL::ResourceState state, 
            SubresourceInfo::AccessFlag accessFlag,
            std::optional<HAL::ColorFormat> shaderVisibleFormat = std::nullopt
        );

        HAL::ResourceState GetSubresourceCombinedReadStates(uint64_t subresourceIndex) const;

        uint64_t HeapOffset = 0;
        bool CanBeAliased = true;

        std::pair<uint64_t, uint64_t> AliasingLifetime = { 
            std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::min() 
        };

    private:
        std::unordered_map<Foundation::Name, PassInfo> mPassInfoMap;
        HAL::ResourceFormat mResourceFormat;
        HAL::ResourceState mExpectedStates = HAL::ResourceState::Common;
        Foundation::Name mResourceName;
        std::vector<Foundation::Name> mAliases;
        uint64_t mSubresourceCount = 0;
        std::string mCombinedResourceNames;

        std::vector<HAL::ResourceState> mSubresourceCombinedReadStates;

    public:
        inline const HAL::ResourceFormat& ResourceFormat() const { return mResourceFormat; }
        inline HAL::ResourceState ExpectedStates() const { return mExpectedStates; }
        inline Foundation::Name ResourceName() const { return mResourceName; }
        inline const auto& Aliases() const { return mAliases; }
        inline auto SubresourceCount() const { return mSubresourceCount; }
        inline auto TotalRequiredMemory() const { return mResourceFormat.ResourceSizeInBytes(); }
        inline auto CombinedResourceNames() const { return mCombinedResourceNames; }
    };

}
