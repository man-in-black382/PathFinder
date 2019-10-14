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

            bool SortAscending(const MemoryRegion& first, const MemoryRegion& second);
            bool SortDescending(const MemoryRegion& first, const MemoryRegion& second);
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
    
            bool SortAscending(const AliasingMetadata& first, const AliasingMetadata& second);
            bool SortDescending(const AliasingMetadata& first, const AliasingMetadata& second);
        };

        using MemoryRegionSet = std::set<AliasingMetadata, decltype(&AliasingMetadata::SortDescending)>;
        using AliasingMetadataSet = std::set<AliasingMetadata, decltype(&AliasingMetadata::SortDescending)>;
        using AliasingMetadataIterator = AliasingMetadataSet::iterator;

        bool TimelinesIntersect(const Timeline& first, const Timeline& second) const;
        Timeline GetTimeline(const PipelineResourceAllocation* allocation) const;

        void AliasWithAlreadyAliasedAllocations(AliasingMetadata& nextAllocation);
        void RemoveAliasedAllocationsFromOriginalList();

        // Containers to be used between function calls to avoid redundant memory allocations
        MemoryRegionSet mIndependentMemoryRegions;
        std::vector<AliasingMetadataIterator> mAlreadyAliasedAllocations;
        uint64_t mCurrentBucketAvailableMemory = 0;

        AliasingMetadataSet mAllocations;
        
        const RenderPassExecutionGraph* mRenderPassGraph;
    };

}
