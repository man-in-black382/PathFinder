#include "PipelineResourceMemoryAliaser.hpp"

#include <limits>

namespace PathFinder
{

    PipelineResourceMemoryAliaser::PipelineResourceMemoryAliaser(const RenderPassExecutionGraph* renderPassGraph)
        : mRenderPassGraph{ renderPassGraph },
        mAllocations{ &AliasingMetadata::SortDescending } {}

    void PipelineResourceMemoryAliaser::AddAllocation(PipelineResourceAllocation* allocation)
    {
        mAllocations.emplace(GetTimeline(allocation), allocation);
    }

    void PipelineResourceMemoryAliaser::Alias()
    {
        if (mAllocations.size() == 0) return;

        if (mAllocations.size() == 1)
        {
            mAllocations.begin()->Allocation->HeapOffset = 0;
            return;
        }
        
        while (!mAllocations.empty())
        {
            auto largestAllocationIt = mAllocations.begin();
            mAvailableMemory = largestAllocationIt->Allocation->Format.ResourceSizeInBytes();

            for (auto allocationIt = largestAllocationIt; allocationIt != mAllocations.end(); ++allocationIt)
            {
                AliasWithAlreadyAliasedAllocations(allocationIt);
            }

            RemoveAliasedAllocationsFromOriginalList();
        }
    }

    bool PipelineResourceMemoryAliaser::TimelinesIntersect(const PipelineResourceMemoryAliaser::Timeline& first, const PipelineResourceMemoryAliaser::Timeline& second) const
    {
        bool startIntersects = first.Start >= second.Start && first.Start <= second.End;
        bool endIntersects = first.End >= second.Start && first.End <= second.End;
        return startIntersects || endIntersects;
    }

    PipelineResourceMemoryAliaser::Timeline PipelineResourceMemoryAliaser::GetTimeline(const PipelineResourceAllocation* allocation) const
    {
        auto firstName = allocation->FirstPassName().ToString();
        auto lastPassName = allocation->LastPassName().ToString();
        return { mRenderPassGraph->IndexOfPass(allocation->FirstPassName()), mRenderPassGraph->IndexOfPass(allocation->LastPassName()) };
    }

    void PipelineResourceMemoryAliaser::AliasWithAlreadyAliasedAllocations(AliasingMetadataIterator nextAllocationIt)
    {
        mNonAliasableMemoryRegionStarts.clear();
        mNonAliasableMemoryRegionEnds.clear();

        uint64_t nextAllocationSize = nextAllocationIt->Allocation->Format.ResourceSizeInBytes();

        if (mAlreadyAliasedAllocations.empty() && nextAllocationSize <= mAvailableMemory)
        {
            nextAllocationIt->Allocation->HeapOffset = 0;
            mAlreadyAliasedAllocations.push_back(nextAllocationIt);
            return;
        }

        MemoryRegion mostFittingMemoryRegion{ 0, -1 };

        // Find memory regions in which we can't place the next allocation, because their allocations
        // are used simultaneously with the next one by some render passed (timelines)
        for (AliasingMetadataIterator alreadyAliasedAllocationIt : mAlreadyAliasedAllocations)
        {
            if (TimelinesIntersect(alreadyAliasedAllocationIt->ResourceTimeline, nextAllocationIt->ResourceTimeline))
            {
                mNonAliasableMemoryRegionStarts.insert(alreadyAliasedAllocationIt->Allocation->HeapOffset);
                mNonAliasableMemoryRegionEnds.insert(alreadyAliasedAllocationIt->Allocation->Format.ResourceSizeInBytes() - 1);
            }
        }

        // Find memory regions in which we can place the next allocation based on previously found unavailable regions.
        // Pick the most fitting region. If next allocation cannot be fit in any free region, skip it.

        bool currentOffsetIsEndpoint = true;
        uint64_t currentOffset = 0;

        auto startIt = mNonAliasableMemoryRegionStarts.begin();
        auto endIt = mNonAliasableMemoryRegionEnds.begin();

        for (; startIt != mNonAliasableMemoryRegionStarts.end() && endIt != mNonAliasableMemoryRegionEnds.end();)
        {
            uint64_t nextStartOffset = *startIt;
            uint64_t nextEndOffset = *endIt;

            bool isRegionEmptyBetweenNextTwoOffsets = nextStartOffset > nextEndOffset && currentOffset <= nextStartOffset && currentOffsetIsEndpoint;

            if (isRegionEmptyBetweenNextTwoOffsets)
            {
                MemoryRegion aliasableMemoryRegion{ currentOffset, nextStartOffset - currentOffset };

                if (aliasableMemoryRegion.Size <= mostFittingMemoryRegion.Size &&
                    nextAllocationSize <= aliasableMemoryRegion.Size)
                {
                    mostFittingMemoryRegion.Offset = currentOffset;
                    mostFittingMemoryRegion.Size = nextAllocationSize;
                }
            }

            if (nextStartOffset < nextEndOffset)
            {
                // Advance to next start offset
                currentOffset = nextStartOffset;
                currentOffsetIsEndpoint = false;
                ++startIt;
            } 
            else {
                // Advance to next end offset
                currentOffset = nextEndOffset;
                currentOffsetIsEndpoint = true;
                ++endIt;
            }
        }

        if (mostFittingMemoryRegion.Size != -1)
        {
            nextAllocationIt->Allocation->HeapOffset = mostFittingMemoryRegion.Offset;
            mAlreadyAliasedAllocations.push_back(nextAllocationIt);
        }
        else if (mNonAliasableMemoryRegionStarts.empty())
        {
            nextAllocationIt->Allocation->HeapOffset = 0;
            mAlreadyAliasedAllocations.push_back(nextAllocationIt);
        }
    }

    void PipelineResourceMemoryAliaser::RemoveAliasedAllocationsFromOriginalList()
    {
        for (AliasingMetadataIterator it : mAlreadyAliasedAllocations)
        {
            mAllocations.erase(it);
        }

        mAlreadyAliasedAllocations.clear();
    }

    PipelineResourceMemoryAliaser::AliasingMetadata::AliasingMetadata(const Timeline& timeline, PipelineResourceAllocation* allocation)
        : ResourceTimeline{ timeline }, Allocation{ allocation } {}

    bool PipelineResourceMemoryAliaser::AliasingMetadata::SortAscending(const AliasingMetadata& first, const AliasingMetadata& second)
    {
        return first.Allocation->Format.ResourceSizeInBytes() < second.Allocation->Format.ResourceSizeInBytes();
    }

    bool PipelineResourceMemoryAliaser::AliasingMetadata::SortDescending(const AliasingMetadata& first, const AliasingMetadata& second)
    {
        return first.Allocation->Format.ResourceSizeInBytes() > second.Allocation->Format.ResourceSizeInBytes();
    }

}
