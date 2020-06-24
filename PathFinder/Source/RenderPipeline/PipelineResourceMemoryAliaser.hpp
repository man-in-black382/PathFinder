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

        struct Timeline
        {
            uint64_t Start;
            uint64_t End;
        };

        struct AliasingMetadata
        {
            Timeline ResourceTimeline;
            PipelineResourceSchedulingInfo* SchedulingInfo;

            AliasingMetadata(const Timeline& timeline, PipelineResourceSchedulingInfo* schedulingInfo);
    
            static bool SortAscending(const AliasingMetadata& first, const AliasingMetadata& second);
            static bool SortDescending(const AliasingMetadata& first, const AliasingMetadata& second);
        };

        using AliasingMetadataSet = std::multiset<AliasingMetadata, decltype(&AliasingMetadata::SortDescending)>;
        using AliasingMetadataIterator = AliasingMetadataSet::iterator;

        bool TimelinesIntersect(const Timeline& first, const Timeline& second) const;
        Timeline GetTimeline(const PipelineResourceSchedulingInfo* allocation) const;
        void FitAliasableMemoryRegion(const MemoryRegion& nextAliasableRegion, uint64_t nextAllocationSize, MemoryRegion& optimalRegion) const;
        void FindCurrentBucketNonAliasableMemoryRegions(AliasingMetadataIterator nextSchedulingInfoIt);
        bool AliasAsFirstAllocation(AliasingMetadataIterator nextSchedulingInfoIt);
        bool AliasAsNonTimelineConflictingAllocation(AliasingMetadataIterator nextSchedulingInfoIt);
        void AliasWithAlreadyAliasedAllocations(AliasingMetadataIterator nextSchedulingInfoIt);
        PipelineResourceSchedulingInfo::PassInfo* GetFirstPassInfo(AliasingMetadataIterator nextSchedulingInfoIt) const;
        void RemoveAliasedAllocationsFromOriginalList();

        std::set<uint64_t, std::less<uint64_t>> mNonAliasableMemoryRegionStarts;
        std::set<uint64_t, std::less<uint64_t>> mNonAliasableMemoryRegionEnds;
        std::vector<AliasingMetadataIterator> mAlreadyAliasedAllocations;
        
        // Memory offset of the current bucket in which aliasing is performed
        uint64_t mGlobalStartOffset = 0;
        uint64_t mAvailableMemory = 0;

        AliasingMetadataSet mSchedulingInfos;
        
        const RenderPassGraph* mRenderPassGraph;
    };

}
