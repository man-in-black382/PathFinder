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

    uint64_t PipelineResourceMemoryAliaser::Alias()
    {
        uint64_t optimalHeapSize = 1;

        if (mAllocations.size() == 0) return optimalHeapSize;

        if (mAllocations.size() == 1)
        {
            mAllocations.begin()->Allocation->HeapOffset = 0;
            optimalHeapSize = mAllocations.begin()->Allocation->ResourceFormat().ResourceSizeInBytes();
            return optimalHeapSize;
        }

        while (!mAllocations.empty())
        {
            auto largestAllocationIt = mAllocations.begin();
            mAvailableMemory = largestAllocationIt->Allocation->ResourceFormat().ResourceSizeInBytes();
            optimalHeapSize += mAvailableMemory;

            for (auto allocationIt = largestAllocationIt; allocationIt != mAllocations.end(); ++allocationIt)
            {
                AliasWithAlreadyAliasedAllocations(allocationIt);
            }

            RemoveAliasedAllocationsFromOriginalList();

            mGlobalStartOffset += mAvailableMemory;
        }

        return optimalHeapSize == 0 ? 1 : optimalHeapSize;
    }

    bool PipelineResourceMemoryAliaser::TimelinesIntersect(const PipelineResourceMemoryAliaser::Timeline& first, const PipelineResourceMemoryAliaser::Timeline& second) const
    {
        bool startIntersects = first.Start >= second.Start && first.Start <= second.End;
        bool endIntersects = first.End >= second.Start && first.End <= second.End;
        return startIntersects || endIntersects;
    }

    PipelineResourceMemoryAliaser::Timeline PipelineResourceMemoryAliaser::GetTimeline(const PipelineResourceAllocation* allocation) const
    {
        return { mRenderPassGraph->IndexOfPass(allocation->FirstPassName()), mRenderPassGraph->IndexOfPass(allocation->LastPassName()) };
    }

    void PipelineResourceMemoryAliaser::FitAliasableMemoryRegion(const MemoryRegion& nextAliasableRegion, uint64_t nextAllocationSize, MemoryRegion& optimalRegion) const
    {
        bool nextRegionValid = nextAliasableRegion.Size > 0;
        bool optimalRegionValid = optimalRegion.Size > 0;
        bool nextRegionIsMoreOptimal = nextAliasableRegion.Size <= optimalRegion.Size || !nextRegionValid || !optimalRegionValid;
        bool allocationFits = nextAllocationSize <= nextAliasableRegion.Size;

        if (allocationFits && nextRegionIsMoreOptimal)
        {
            optimalRegion.Offset = nextAliasableRegion.Offset;
            optimalRegion.Size = nextAllocationSize;
        }
    }

    void PipelineResourceMemoryAliaser::FindNonAliasableMemoryRegions(AliasingMetadataIterator nextAllocationIt)
    {
        mNonAliasableMemoryRegionStarts.clear();
        mNonAliasableMemoryRegionEnds.clear();

        // Find memory regions in which we can't place the next allocation, because their allocations
        // are used simultaneously with the next one by some render passed (timelines)
        for (AliasingMetadataIterator alreadyAliasedAllocationIt : mAlreadyAliasedAllocations)
        {
            if (TimelinesIntersect(alreadyAliasedAllocationIt->ResourceTimeline, nextAllocationIt->ResourceTimeline))
            {
                uint64_t startByteIndex = alreadyAliasedAllocationIt->Allocation->HeapOffset;
                uint64_t endByteIndex = startByteIndex + alreadyAliasedAllocationIt->Allocation->ResourceFormat().ResourceSizeInBytes() - 1;

                mNonAliasableMemoryRegionStarts.insert(startByteIndex);
                mNonAliasableMemoryRegionEnds.insert(endByteIndex);
            }
        }
    }

    bool PipelineResourceMemoryAliaser::AliasAsFirstAllocation(AliasingMetadataIterator nextAllocationIt)
    {
        if (mAlreadyAliasedAllocations.empty() && nextAllocationIt->Allocation->ResourceFormat().ResourceSizeInBytes() <= mAvailableMemory)
        {
            nextAllocationIt->Allocation->HeapOffset = mGlobalStartOffset;
            mAlreadyAliasedAllocations.push_back(nextAllocationIt);
            return true;
        }

        return false;
    }

    bool PipelineResourceMemoryAliaser::AliasAsNonTimelineConflictingAllocation(AliasingMetadataIterator nextAllocationIt)
    {
        if (mNonAliasableMemoryRegionStarts.empty())
        {
            nextAllocationIt->Allocation->HeapOffset = mGlobalStartOffset;
            mAlreadyAliasedAllocations.push_back(nextAllocationIt);
            return true;
        }

        return false;
    }

    void PipelineResourceMemoryAliaser::AliasWithAlreadyAliasedAllocations(AliasingMetadataIterator nextAllocationIt)
    {
        // Bail out if there is nothing to alias with
        if (AliasAsFirstAllocation(nextAllocationIt)) return;

        FindNonAliasableMemoryRegions(nextAllocationIt);

        // Bail out if there is no timeline conflicts with already aliased resources
        if (AliasAsNonTimelineConflictingAllocation(nextAllocationIt)) return;

        // Find memory regions in which we can place the next allocation based on previously found unavailable regions.
        // Pick the most fitting region. If next allocation cannot be fit in any free region, skip it.

        uint64_t localOffset = mGlobalStartOffset;
        uint16_t overlappingMemoryRegionsCount = 0;
        uint64_t nextAllocationSize = nextAllocationIt->Allocation->ResourceFormat().ResourceSizeInBytes();

        auto startIt = mNonAliasableMemoryRegionStarts.begin();
        auto endIt = mNonAliasableMemoryRegionEnds.begin();

        MemoryRegion mostFittingMemoryRegion{ 0, 0 };

        // Handle first free region from start of the bucket to first non-aliasable region if it exists
        if (!mNonAliasableMemoryRegionStarts.empty())
        {
            uint64_t regionSize = *startIt;
            MemoryRegion nextAliasableMemoryRegion{ 0, regionSize };
            FitAliasableMemoryRegion(nextAliasableMemoryRegion, nextAllocationSize, mostFittingMemoryRegion);

            localOffset = *startIt;
            ++startIt;
            ++overlappingMemoryRegionsCount;
        }

        // Search for free aliasable memory regions between non-aliasable regions
        for (; startIt != mNonAliasableMemoryRegionStarts.end() && endIt != mNonAliasableMemoryRegionEnds.end();)
        {
            uint64_t nextStartOffset = *startIt;
            uint64_t nextEndOffset = *endIt;

            bool nextRegionIsEmpty = overlappingMemoryRegionsCount == 0;
            bool nextPointIsStartPoint = nextStartOffset < nextEndOffset;

            if (nextPointIsStartPoint)
            {
                ++startIt;
                ++overlappingMemoryRegionsCount;
            }
            else {
                ++endIt;
                --overlappingMemoryRegionsCount;
            }

            if (nextRegionIsEmpty && nextPointIsStartPoint)
            {
                MemoryRegion nextAliasableMemoryRegion{ localOffset, nextStartOffset - localOffset };
                FitAliasableMemoryRegion(nextAliasableMemoryRegion, nextAllocationSize, mostFittingMemoryRegion);
            }

            localOffset = nextPointIsStartPoint ? nextStartOffset : nextEndOffset;
        }

        // Handle last free region from end of last non-aliasable region to bucket end
        if (!mNonAliasableMemoryRegionEnds.empty())
        {
            auto lastNonAliasableRegionEndIt = --mNonAliasableMemoryRegionEnds.end();

            uint64_t lastEmptyRegionOffset = *lastNonAliasableRegionEndIt + 1; // One past the end offset of non-aliasable region
            uint64_t lastEmptyRegionSize = mAvailableMemory - lastEmptyRegionOffset;
            MemoryRegion nextAliasableMemoryRegion{ lastEmptyRegionOffset, lastEmptyRegionSize };

            FitAliasableMemoryRegion(nextAliasableMemoryRegion, nextAllocationSize, mostFittingMemoryRegion);
        }

        // If we found a fitting aliasable memory region update allocation with an offset
        if (mostFittingMemoryRegion.Size > 0)
        {
            // Let's pick largest resource as a source resource for aliasing.
            // DirectX spec doesn't say anything on how to choose from several 
            // available source resources

            nextAllocationIt->Allocation->AliasingSource = mAllocations.begin()->Allocation;
            nextAllocationIt->Allocation->HeapOffset = mostFittingMemoryRegion.Offset;
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
        return first.Allocation->ResourceFormat().ResourceSizeInBytes() < second.Allocation->ResourceFormat().ResourceSizeInBytes();
    }

    bool PipelineResourceMemoryAliaser::AliasingMetadata::SortDescending(const AliasingMetadata& first, const AliasingMetadata& second)
    {
        return first.Allocation->ResourceFormat().ResourceSizeInBytes() > second.Allocation->ResourceFormat().ResourceSizeInBytes();
    }

}
