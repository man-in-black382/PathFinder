#include "PipelineResourceMemoryAliaser.hpp"

#include <limits>

#include "../Foundation/StringUtils.hpp"

namespace PathFinder
{

    PipelineResourceMemoryAliaser::PipelineResourceMemoryAliaser(const RenderPassExecutionGraph* renderPassGraph)
        : mRenderPassGraph{ renderPassGraph },
        mSchedulingInfos{ &AliasingMetadata::SortDescending } {}

    void PipelineResourceMemoryAliaser::AddSchedulingInfo(PipelineResourceSchedulingInfo* scheudlingInfo)
    {
        mSchedulingInfos.emplace(GetTimeline(scheudlingInfo), scheudlingInfo);
    }

    uint64_t PipelineResourceMemoryAliaser::Alias()
    {
        uint64_t optimalHeapSize = 0;

        if (mSchedulingInfos.size() == 0) 
        {
            return 1;
        }

        if (mSchedulingInfos.size() == 1)
        {
            mSchedulingInfos.begin()->SchedulingInfo->AliasingInfo.HeapOffset = 0;
            optimalHeapSize = mSchedulingInfos.begin()->SchedulingInfo->ResourceFormat().ResourceSizeInBytes();
            return optimalHeapSize;
        }

        while (!mSchedulingInfos.empty())
        {
            auto largestAllocationIt = mSchedulingInfos.begin();
            mAvailableMemory = largestAllocationIt->SchedulingInfo->ResourceFormat().ResourceSizeInBytes();
            optimalHeapSize += mAvailableMemory;

            for (auto schedulingInfoIt = largestAllocationIt; schedulingInfoIt != mSchedulingInfos.end(); ++schedulingInfoIt)
            {
                AliasWithAlreadyAliasedAllocations(schedulingInfoIt);
            }

            RemoveAliasedAllocationsFromOriginalList();

            mGlobalStartOffset += mAvailableMemory;
        }

        return optimalHeapSize == 0 ? 1 : optimalHeapSize;
    }

    bool PipelineResourceMemoryAliaser::IsEmpty() const
    {
        return mSchedulingInfos.empty();
    }

    bool PipelineResourceMemoryAliaser::TimelinesIntersect(const PipelineResourceMemoryAliaser::Timeline& first, const PipelineResourceMemoryAliaser::Timeline& second) const
    {
        return first.Start <= second.End && second.Start <= first.End;
    }

    PipelineResourceMemoryAliaser::Timeline PipelineResourceMemoryAliaser::GetTimeline(const PipelineResourceSchedulingInfo* schedulingInfo) const
    {
        return { schedulingInfo->FirstPassGraphNode().ContextualExecutionIndex, schedulingInfo->LastPassGraphNode().ContextualExecutionIndex };
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

    void PipelineResourceMemoryAliaser::FindCurrentBucketNonAliasableMemoryRegions(AliasingMetadataIterator nextAllocationIt)
    {
        mNonAliasableMemoryRegionStarts.clear();
        mNonAliasableMemoryRegionEnds.clear();

        // Find memory regions in which we can't place the next allocation, because their allocations
        // are used simultaneously with the next one by some render passed (timelines)
        for (AliasingMetadataIterator alreadyAliasedAllocationIt : mAlreadyAliasedAllocations)
        {
            if (TimelinesIntersect(alreadyAliasedAllocationIt->ResourceTimeline, nextAllocationIt->ResourceTimeline))
            {
                // Heap offset stored in AliasingInfo.HeapOffset is relative to heap beginning, 
                // but the algorithm requires non-aliasable memory region to be 
                // relative to current global offset, which is an offset of the current memory bucket we're aliasing resources in,
                // therefore we have to subtract current global offset
                uint64_t startByteIndex = alreadyAliasedAllocationIt->SchedulingInfo->AliasingInfo.HeapOffset - mGlobalStartOffset;
                uint64_t endByteIndex = startByteIndex + alreadyAliasedAllocationIt->SchedulingInfo->ResourceFormat().ResourceSizeInBytes() - 1;

                mNonAliasableMemoryRegionStarts.insert(startByteIndex);
                mNonAliasableMemoryRegionEnds.insert(endByteIndex);
            }
        }
    }

    bool PipelineResourceMemoryAliaser::AliasAsFirstAllocation(AliasingMetadataIterator nextAllocationIt)
    {
        if (mAlreadyAliasedAllocations.empty() && nextAllocationIt->SchedulingInfo->ResourceFormat().ResourceSizeInBytes() <= mAvailableMemory)
        {
            nextAllocationIt->SchedulingInfo->AliasingInfo.HeapOffset = mGlobalStartOffset;
            mAlreadyAliasedAllocations.push_back(nextAllocationIt);
            return true;
        }

        return false;
    }

    bool PipelineResourceMemoryAliaser::AliasAsNonTimelineConflictingAllocation(AliasingMetadataIterator nextAllocationIt)
    {
        if (mNonAliasableMemoryRegionStarts.empty())
        {
            nextAllocationIt->SchedulingInfo->AliasingInfo.HeapOffset = mGlobalStartOffset;
            mAlreadyAliasedAllocations.push_back(nextAllocationIt);
            return true;
        }

        return false;
    }

    void PipelineResourceMemoryAliaser::AliasWithAlreadyAliasedAllocations(AliasingMetadataIterator nextSchedulingInfoIt)
    {
        // Bail out if there is nothing to alias with
        if (AliasAsFirstAllocation(nextSchedulingInfoIt)) return;

        FindCurrentBucketNonAliasableMemoryRegions(nextSchedulingInfoIt);

        // Bail out if there is no timeline conflicts with already aliased resources
        if (AliasAsNonTimelineConflictingAllocation(nextSchedulingInfoIt)) return;

        // Find memory regions in which we can place the next allocation based on previously found unavailable regions.
        // Pick the most fitting region. If next allocation cannot be fit in any free region, skip it.

        uint64_t localOffset = 0;
        uint16_t overlappingMemoryRegionsCount = 0;
        uint64_t nextAllocationSize = nextSchedulingInfoIt->SchedulingInfo->ResourceFormat().ResourceSizeInBytes();

        auto startIt = mNonAliasableMemoryRegionStarts.begin();
        auto endIt = mNonAliasableMemoryRegionEnds.begin();

        MemoryRegion mostFittingMemoryRegion{ 0, 0 };

        // Handle first free region from start of the bucket to first non-aliasable region if it exists
        if (!mNonAliasableMemoryRegionStarts.empty())
        {
            // Consider first aliasable region size to be from 0 to first non-aliasable region start
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
            uint64_t nextEndOffset = *endIt + 1; // Move past the index of non-aliasable region end byte 

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

            // If we're using 
            localOffset = nextPointIsStartPoint ? nextStartOffset : nextEndOffset;
        }

        // Handle last free region from end of last non-aliasable region to bucket end
        if (!mNonAliasableMemoryRegionEnds.empty())
        {
            auto lastNonAliasableRegionEndIt = std::prev(mNonAliasableMemoryRegionEnds.end());

            // Check if a free, aliasable memory region exists after last non-aliasable memory region
            // and before end of this memory bucket and whether that region can fit requested allocation.
            uint64_t lastEmptyRegionOffset = *lastNonAliasableRegionEndIt + 1; // One past the end offset of non-aliasable region
            uint64_t lastEmptyRegionSize = mAvailableMemory - lastEmptyRegionOffset;
            MemoryRegion nextAliasableMemoryRegion{ lastEmptyRegionOffset, lastEmptyRegionSize };

            FitAliasableMemoryRegion(nextAliasableMemoryRegion, nextAllocationSize, mostFittingMemoryRegion);
        }

        // If we found a fitting aliasable memory region update allocation with an offset
        if (mostFittingMemoryRegion.Size > 0)
        {
            // ??? DirectX spec doesn't say anything on how to choose from several 
            // available source resources for aliasing
            // 
            // Do not use any source in aliasing barriers for the moment. 
            // Probably won't hurt on most hardware/drivers, but need to
            // figure out a way to check the actual behavior
            //
            //nextAllocationIt->Allocation->AliasingSource = mAllocations.begin()->Allocation;

            PipelineResourceSchedulingInfo::AliasingMetadata& aliasingInfo = nextSchedulingInfoIt->SchedulingInfo->AliasingInfo;

            // Offset calculations were made in a frame relative to the current memory bucket.
            // Now we need to adjust it to be relative to the heap start.
            aliasingInfo.HeapOffset = mGlobalStartOffset + mostFittingMemoryRegion.Offset;
            aliasingInfo.NeedsAliasingBarrier = true;

            // We aliased something with the first resource in the current memory bucket
            // so it's no longer a single occupant of this memory region, therefore it now
            // needs an aliasing barrier. If the first resource is a single resource on this
            // memory region then this code branch will never be hit and we will avoid a barrier for it.
            mAlreadyAliasedAllocations.front()->SchedulingInfo->AliasingInfo.NeedsAliasingBarrier = true;

            mAlreadyAliasedAllocations.push_back(nextSchedulingInfoIt);
        }
    }

    void PipelineResourceMemoryAliaser::RemoveAliasedAllocationsFromOriginalList()
    {
        for (AliasingMetadataIterator it : mAlreadyAliasedAllocations)
        {
            mSchedulingInfos.erase(it);
        }

        mAlreadyAliasedAllocations.clear();
    }

    PipelineResourceMemoryAliaser::AliasingMetadata::AliasingMetadata(const Timeline& timeline, PipelineResourceSchedulingInfo* schedulingInfo)
        : ResourceTimeline{ timeline }, SchedulingInfo{ schedulingInfo } {}

    bool PipelineResourceMemoryAliaser::AliasingMetadata::SortAscending(const AliasingMetadata& first, const AliasingMetadata& second)
    {
        return first.SchedulingInfo->ResourceFormat().ResourceSizeInBytes() < second.SchedulingInfo->ResourceFormat().ResourceSizeInBytes();
    }

    bool PipelineResourceMemoryAliaser::AliasingMetadata::SortDescending(const AliasingMetadata& first, const AliasingMetadata& second)
    {
        return first.SchedulingInfo->ResourceFormat().ResourceSizeInBytes() > second.SchedulingInfo->ResourceFormat().ResourceSizeInBytes();
    }

}
