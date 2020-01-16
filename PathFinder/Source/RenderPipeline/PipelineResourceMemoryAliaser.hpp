#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include "PipelineResourceSchedulingInfo.hpp"
#include "RenderPassExecutionGraph.hpp"

#include <set>

namespace PathFinder
{

    class PipelineResourceMemoryAliaser
    {
    public:
        PipelineResourceMemoryAliaser(const RenderPassExecutionGraph* renderPassGraph);

        void AddAllocation(PipelineResourceSchedulingInfo* allocation);
        uint64_t Alias();

    private:
        struct MemoryRegion
        {
            uint64_t Offset;
            uint64_t Size;
        };

        struct Timeline
        {
            uint32_t Start;
            uint32_t End;
        };

        struct AliasingMetadata
        {
            Timeline ResourceTimeline;
            PipelineResourceSchedulingInfo* Allocation;

            AliasingMetadata(const Timeline& timeline, PipelineResourceSchedulingInfo* allocation);
    
            static bool SortAscending(const AliasingMetadata& first, const AliasingMetadata& second);
            static bool SortDescending(const AliasingMetadata& first, const AliasingMetadata& second);
        };

        using AliasingMetadataSet = std::multiset<AliasingMetadata, decltype(&AliasingMetadata::SortDescending)>;
        using AliasingMetadataIterator = AliasingMetadataSet::iterator;

        bool TimelinesIntersect(const Timeline& first, const Timeline& second) const;
        Timeline GetTimeline(const PipelineResourceSchedulingInfo* allocation) const;
        void FitAliasableMemoryRegion(const MemoryRegion& nextAliasableRegion, uint64_t nextAllocationSize, MemoryRegion& optimalRegion) const;
        void FindNonAliasableMemoryRegions(AliasingMetadataIterator nextAllocationIt);
        bool AliasAsFirstAllocation(AliasingMetadataIterator nextAllocationIt);
        bool AliasAsNonTimelineConflictingAllocation(AliasingMetadataIterator nextAllocationIt);
        void AliasWithAlreadyAliasedAllocations(AliasingMetadataIterator nextAllocationIt);
        void RemoveAliasedAllocationsFromOriginalList();

        // Containers to be used between function calls to avoid redundant memory allocations
        std::set<uint32_t, std::less<uint32_t>> mNonAliasableMemoryRegionStarts;
        std::set<uint32_t, std::less<uint32_t>> mNonAliasableMemoryRegionEnds;
        std::vector<AliasingMetadataIterator> mAlreadyAliasedAllocations;
        uint64_t mGlobalStartOffset = 0;
        uint64_t mAvailableMemory = 0;

        AliasingMetadataSet mAllocations;
        
        const RenderPassExecutionGraph* mRenderPassGraph;
    };

}
