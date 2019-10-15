#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include "PipelineResourceAllocation.hpp"
#include "RenderPassExecutionGraph.hpp"

#include <set>

namespace PathFinder
{

    class PipelineResourceMemoryAliaser
    {
    public:
        PipelineResourceMemoryAliaser(const RenderPassExecutionGraph* renderPassGraph);

        void AddAllocation(PipelineResourceAllocation* allocation);
        void Alias();

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
            PipelineResourceAllocation* Allocation;

            AliasingMetadata(const Timeline& timeline, PipelineResourceAllocation* allocation);
    
            static bool SortAscending(const AliasingMetadata& first, const AliasingMetadata& second);
            static bool SortDescending(const AliasingMetadata& first, const AliasingMetadata& second);
        };

        using AliasingMetadataSet = std::multiset<AliasingMetadata, decltype(&AliasingMetadata::SortDescending)>;
        using AliasingMetadataIterator = AliasingMetadataSet::iterator;

        bool TimelinesIntersect(const Timeline& first, const Timeline& second) const;
        Timeline GetTimeline(const PipelineResourceAllocation* allocation) const;

        void AliasWithAlreadyAliasedAllocations(AliasingMetadataIterator nextAllocationIt);
        void RemoveAliasedAllocationsFromOriginalList();

        // Containers to be used between function calls to avoid redundant memory allocations
        std::set<uint32_t, std::less<uint32_t>> mNonAliasableMemoryRegionStarts;
        std::set<uint32_t, std::less<uint32_t>> mNonAliasableMemoryRegionEnds;
        std::vector<AliasingMetadataIterator> mAlreadyAliasedAllocations;
        uint64_t mAvailableMemory = 0;

        AliasingMetadataSet mAllocations;
        
        const RenderPassExecutionGraph* mRenderPassGraph;
    };

}
