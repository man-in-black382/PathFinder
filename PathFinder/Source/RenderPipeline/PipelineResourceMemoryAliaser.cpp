#include "PipelineResourceMemoryAliaser.hpp"

namespace PathFinder
{

    PipelineResourceMemoryAliaser::PipelineResourceMemoryAliaser(const RenderPassExecutionGraph* renderPassGraph)
        : mRenderPassGraph{ renderPassGraph },
        mAllocations{ &AliasingMetadata::SortDescending },
        mIndependentMemoryRegions{ &MemoryRegion::SortAscending } {}

    void PipelineResourceMemoryAliaser::AddAllocation(PipelineResourceAllocation* allocation)
    {
        mAllocations.insert(AliasingMetadata{ GetTimeline(allocation), allocation });
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
            mCurrentBucketAvailableMemory = largestAllocationIt->Allocation->Format.ResourceSizeInBytes();

            for (auto allocationIt = largestAllocationIt; allocationIt != mAllocations.end(); ++allocationIt)
            {
                AliasWithAlreadyAliasedAllocations(allocationIt);
            }

            RemoveAliasedAllocationsFromOriginalList();
        }
    }

    bool PipelineResourceMemoryAliaser::TimelinesIntersect(const Timeline& first, const Timeline& second) const
    {
        bool startIntersects = first.Start >= second.Start && first.Start <= second.End;
        bool endIntersects = first.End >= second.Start && first.End <= second.End;
        return startIntersects || endIntersects;
    }

    PipelineResourceMemoryAliaser::Timeline PipelineResourceMemoryAliaser::GetTimeline(const PipelineResourceAllocation* allocation) const
    {
        return { mRenderPassGraph->IndexOfPass(allocation->FirstPassName()), mRenderPassGraph->IndexOfPass(allocation->LastPassName()) };
    }

    void PipelineResourceMemoryAliaser::AliasWithAlreadyAliasedAllocations(AliasingMetadataIterator nextAllocationIt)
    {
        mIndependentMemoryRegions.clear();

        for (AliasingMetadataIterator it : mAlreadyAliasedAllocations)
        {
            
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

    bool PipelineResourceMemoryAliaser::AliasingMetadata::SortAscending(const AliasingMetadata& first, const AliasingMetadata& second)
    {
        return first.Allocation->Format.ResourceSizeInBytes() < second.Allocation->Format.ResourceSizeInBytes();
    }

    bool PipelineResourceMemoryAliaser::AliasingMetadata::SortDescending(const AliasingMetadata& first, const AliasingMetadata& second)
    {
        return first.Allocation->Format.ResourceSizeInBytes() > second.Allocation->Format.ResourceSizeInBytes();
    }

    bool PipelineResourceMemoryAliaser::MemoryRegion::SortAscending(const MemoryRegion& first, const MemoryRegion& second)
    {
        return first.Size < second.Size;
    }

    bool PipelineResourceMemoryAliaser::MemoryRegion::SortDescending(const MemoryRegion& first, const MemoryRegion& second)
    {
        return first.Size > second.Size;
    }

}
