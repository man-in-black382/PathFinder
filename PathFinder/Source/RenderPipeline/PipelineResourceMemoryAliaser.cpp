#include "PipelineResourceMemoryAliaser.hpp"

#include <limits>
#include <algorithm>

#include <Foundation/StringUtils.hpp>

namespace PathFinder
{

    PipelineResourceMemoryAliaser::PipelineResourceMemoryAliaser(const RenderPassGraph* renderPassGraph)
        : mRenderPassGraph{ renderPassGraph },
        mSchedulingInfos{ &AliasingMetadata::SortDescending } {}

    void PipelineResourceMemoryAliaser::AddSchedulingInfo(PipelineResourceSchedulingInfo* scheudlingInfo)
    {
        mSchedulingInfos.emplace(scheudlingInfo);
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
            mSchedulingInfos.begin()->SchedulingInfo->HeapOffset = 0;
            optimalHeapSize = mSchedulingInfos.begin()->SchedulingInfo->TotalRequiredMemory();
            return optimalHeapSize;
        }

        while (!mSchedulingInfos.empty())
        {
            auto largestAllocationIt = mSchedulingInfos.begin();
            mAvailableMemory = largestAllocationIt->SchedulingInfo->TotalRequiredMemory();
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

    bool PipelineResourceMemoryAliaser::TimelinesIntersect(const PipelineResourceSchedulingInfo& first, const PipelineResourceSchedulingInfo& second) const
    {
        return first.AliasingLifetime.first <= second.AliasingLifetime.second &&
            second.AliasingLifetime.first <= first.AliasingLifetime.second;
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

    void PipelineResourceMemoryAliaser::FindCurrentBucketNonAliasableMemoryRegions(AliasingMetadataIterator nextSchedulingInfoIt)
    {
        mNonAliasableMemoryOffsets.clear();
        mNonAliasableMemoryOffsets.push_back({ 0, MemoryOffsetType::End });

        // Find memory regions in which we can't place the next allocation, because their allocations
        // are used simultaneously with the next one by some render passed (timelines)
        for (AliasingMetadataIterator alreadyAliasedAllocationIt : mAlreadyAliasedAllocations)
        {
            if (TimelinesIntersect(*alreadyAliasedAllocationIt->SchedulingInfo, *nextSchedulingInfoIt->SchedulingInfo))
            {
                // Heap offset stored in AliasingInfo.HeapOffset is relative to heap beginning, 
                // but the algorithm requires non-aliasable memory region to be 
                // relative to current global offset, which is an offset of the current memory bucket we're aliasing resources in,
                // therefore we have to subtract current global offset
                uint64_t startByteIndex = alreadyAliasedAllocationIt->SchedulingInfo->HeapOffset - mGlobalStartOffset;
                uint64_t endByteIndex = startByteIndex + alreadyAliasedAllocationIt->SchedulingInfo->TotalRequiredMemory();

                mNonAliasableMemoryOffsets.push_back({ startByteIndex, MemoryOffsetType::Start });
                mNonAliasableMemoryOffsets.push_back({ endByteIndex, MemoryOffsetType::End });
            }
        }

        mNonAliasableMemoryOffsets.push_back({ mAvailableMemory, MemoryOffsetType::Start });

        std::sort(mNonAliasableMemoryOffsets.begin(), mNonAliasableMemoryOffsets.end(), [](auto& offset1, auto& offset2) -> bool
        {
            return offset1.first < offset2.first;
        });
    }

    bool PipelineResourceMemoryAliaser::AliasAsFirstAllocation(AliasingMetadataIterator nextSchedulingInfoIt)
    {
        if (mAlreadyAliasedAllocations.empty() && nextSchedulingInfoIt->SchedulingInfo->TotalRequiredMemory() <= mAvailableMemory)
        {
            nextSchedulingInfoIt->SchedulingInfo->HeapOffset = mGlobalStartOffset;
            mAlreadyAliasedAllocations.push_back(nextSchedulingInfoIt);
            return true;
        }

        return false;
    }

    void PipelineResourceMemoryAliaser::AliasWithAlreadyAliasedAllocations(AliasingMetadataIterator nextSchedulingInfoIt)
    {
        // Bail out if there is nothing to alias with
        if (AliasAsFirstAllocation(nextSchedulingInfoIt)) return;

        FindCurrentBucketNonAliasableMemoryRegions(nextSchedulingInfoIt);

        // Find memory regions in which we can place the next allocation based on previously found unavailable regions.
        // Pick the most fitting region. If next allocation cannot be fit in any free region, skip it.
        uint64_t nextAllocationSize = nextSchedulingInfoIt->SchedulingInfo->TotalRequiredMemory();
        MemoryRegion mostFittingMemoryRegion{ 0, 0 };
        int64_t overlapCounter = 0;

        for (auto i = 0u; i < mNonAliasableMemoryOffsets.size() - 1; ++i)
        {
            const auto& [currentOffset, currentType] = mNonAliasableMemoryOffsets[i];
            const auto& [nextOffset, nextType] = mNonAliasableMemoryOffsets[i + 1];

            overlapCounter += currentType == MemoryOffsetType::Start ? 1 : -1;
            overlapCounter = std::max(overlapCounter, 0ll);

            bool reachedAliasableRegion = 
                overlapCounter == 0 && 
                currentType == MemoryOffsetType::End && 
                nextType == MemoryOffsetType::Start;

            if (reachedAliasableRegion)
            {
                MemoryRegion nextAliasableMemoryRegion{ currentOffset, nextOffset - currentOffset };
                FitAliasableMemoryRegion(nextAliasableMemoryRegion, nextAllocationSize, mostFittingMemoryRegion);
            }
        }

        // If we found a fitting aliasable memory region update allocation with an offset
        if (mostFittingMemoryRegion.Size > 0)
        {
            // Offset calculations were made in a frame relative to the current memory bucket.
            // Now we need to adjust it to be relative to the heap start.
            nextSchedulingInfoIt->SchedulingInfo->HeapOffset = mGlobalStartOffset + mostFittingMemoryRegion.Offset;

            PipelineResourceSchedulingInfo::PassInfo* firstPassInfo = GetFirstPassInfo(nextSchedulingInfoIt);
            firstPassInfo->NeedsAliasingBarrier = true;

            // We aliased something with the first resource in the current memory bucket
            // so it's no longer a single occupant of this memory region, therefore it now
            // needs an aliasing barrier. If the first resource is a single resource on this
            // memory region then this code branch will never be hit and we will avoid a barrier for it.
            firstPassInfo = GetFirstPassInfo(mAlreadyAliasedAllocations.front());
            firstPassInfo->NeedsAliasingBarrier = true;

            mAlreadyAliasedAllocations.push_back(nextSchedulingInfoIt);
        }
    }

    PipelineResourceSchedulingInfo::PassInfo* PipelineResourceMemoryAliaser::GetFirstPassInfo(AliasingMetadataIterator schedulingInfoIt) const
    {
        const RenderPassGraph::Node* firstNode = mRenderPassGraph->NodesInGlobalExecutionOrder().at(schedulingInfoIt->SchedulingInfo->AliasingLifetime.first);
        return schedulingInfoIt->SchedulingInfo->GetInfoForPass(firstNode->PassMetadata().Name);
    }

    void PipelineResourceMemoryAliaser::RemoveAliasedAllocationsFromOriginalList()
    {
        for (AliasingMetadataIterator it : mAlreadyAliasedAllocations)
        {
            mSchedulingInfos.erase(it);
        }

        mAlreadyAliasedAllocations.clear();
    }

    PipelineResourceMemoryAliaser::AliasingMetadata::AliasingMetadata(PipelineResourceSchedulingInfo* schedulingInfo)
        : SchedulingInfo{ schedulingInfo } {}

    bool PipelineResourceMemoryAliaser::AliasingMetadata::SortAscending(const AliasingMetadata& first, const AliasingMetadata& second)
    {
        return first.SchedulingInfo->TotalRequiredMemory() < second.SchedulingInfo->TotalRequiredMemory();
    }

    bool PipelineResourceMemoryAliaser::AliasingMetadata::SortDescending(const AliasingMetadata& first, const AliasingMetadata& second)
    {
        return first.SchedulingInfo->TotalRequiredMemory() > second.SchedulingInfo->TotalRequiredMemory();
    }

}
