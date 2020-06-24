#include "ResourceBarrier.hpp"
#include "Utils.h"

namespace HAL
{
    ResourceBarrier::~ResourceBarrier() {}

    ResourceTransitionBarrier::ResourceTransitionBarrier(
        ResourceState beforeStateMask,
        ResourceState afterStateMask,
        const Resource* resource,
        std::optional<uint64_t> subresourceIndex)
        :
        mResource(resource), mBeforeStates(beforeStateMask), mAfterStates(afterStateMask)
    {
        mDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        mDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        mDesc.Transition.Subresource = subresourceIndex.value_or(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        mDesc.Transition.StateBefore = D3DResourceState(beforeStateMask);
        mDesc.Transition.StateAfter = D3DResourceState(afterStateMask);
        mDesc.Transition.pResource = resource->D3DResource();
    }

    std::pair<ResourceTransitionBarrier, ResourceTransitionBarrier> ResourceTransitionBarrier::Split() const
    {
        ResourceTransitionBarrier before = *this;
        ResourceTransitionBarrier after = *this;
        before.mDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
        after.mDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
        return { before, after };
    }

    ResourceAliasingBarrier::ResourceAliasingBarrier(const Resource* source, const Resource* destination)
        : mSource{ source }, mDestination{ destination }
    {
        mDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        mDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        mDesc.Aliasing.pResourceBefore = source ? source->D3DResource() : nullptr;
        mDesc.Aliasing.pResourceAfter = destination ? destination->D3DResource() : nullptr;
    }

    UnorderedAccessResourceBarrier::UnorderedAccessResourceBarrier(const Resource* resource)
        : mResource{ resource }
    {
        mDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        mDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        mDesc.UAV.pResource = resource ? resource->D3DResource() : nullptr;
    }

    void ResourceBarrierCollection::AddBarrier(const ResourceBarrier& barrier)
    {
        mD3DBarriers.push_back(barrier.D3DBarrier());
    }

    void ResourceBarrierCollection::AddBarriers(const ResourceBarrierCollection& barriers)
    {
        mD3DBarriers.insert(mD3DBarriers.end(), barriers.mD3DBarriers.begin(), barriers.mD3DBarriers.end());
    }

}

