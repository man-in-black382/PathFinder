#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include "PipelineResourceSchedulingInfo.hpp"
#include "RenderPassGraph.hpp"

#include <set>

namespace PathFinder
{

    // Helper class to determine memory aliasing properties
    class PipelineResourceMemoryAliaser
    {
    public:
        PipelineResourceMemoryAliaser(const RenderPassGraph* renderPassGraph);

        void AddSchedulingInfo(PipelineResourceSchedulingInfo* schedulingInfo);
        uint64_t Alias();
        bool IsEmpty() const;

    private:
        struct MemoryRegion
        {
            uint64_t Offset;
            uint64_t Size;
        };

        enum class MemoryOffsetType
        {
            Start, End
        };

        using MemoryOffset = std::pair<uint64_t, MemoryOffsetType>;

        struct AliasingMetadata
        {
            PipelineResourceSchedulingInfo* SchedulingInfo;

            AliasingMetadata(PipelineResourceSchedulingInfo* schedulingInfo);
    
            static bool SortAscending(const AliasingMetadata& first, const AliasingMetadata& second);
            static bool SortDescending(const AliasingMetadata& first, const AliasingMetadata& second);
        };

        using AliasingMetadataSet = std::multiset<AliasingMetadata, decltype(&AliasingMetadata::SortDescending)>;
        using AliasingMetadataIterator = AliasingMetadataSet::iterator;

        bool TimelinesIntersect(const PipelineResourceSchedulingInfo& first, const PipelineResourceSchedulingInfo& second) const;
        void FitAliasableMemoryRegion(const MemoryRegion& nextAliasableRegion, uint64_t nextAllocationSize, MemoryRegion& optimalRegion) const;
        void FindCurrentBucketNonAliasableMemoryRegions(AliasingMetadataIterator nextSchedulingInfoIt);
        bool AliasAsFirstAllocation(AliasingMetadataIterator nextSchedulingInfoIt);
        void AliasWithAlreadyAliasedAllocations(AliasingMetadataIterator nextSchedulingInfoIt);
        PipelineResourceSchedulingInfo::PassInfo* GetFirstPassInfo(AliasingMetadataIterator nextSchedulingInfoIt) const;
        void RemoveAliasedAllocationsFromOriginalList();

        std::vector<MemoryOffset> mNonAliasableMemoryOffsets;
        std::vector<AliasingMetadataIterator> mAlreadyAliasedAllocations;
        
        // Memory offset of the current bucket in which aliasing is performed
        uint64_t mGlobalStartOffset = 0;
        uint64_t mAvailableMemory = 0;

        AliasingMetadataSet mSchedulingInfos;
        
        const RenderPassGraph* mRenderPassGraph;
    };

}
