#include "ResourceBarrier.hpp"
#include "Utils.h"

namespace HAL
{

    ResourceBarrier::~ResourceBarrier()
    {

    }

    ResourceTransitionBarrier::ResourceTransitionBarrier(ResourceState beforeStateMask, ResourceState afterStateMask, const Resource* resource)
        : mResource(resource), mBeforeStates(beforeStateMask), mAfterStates(afterStateMask)
    {
        mDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        mDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        mDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        mDesc.Transition.StateBefore = D3D12_RESOURCE_STATES(beforeStateMask);
        mDesc.Transition.StateAfter = D3D12_RESOURCE_STATES(afterStateMask);
        mDesc.Transition.pResource = resource->D3DPtr();
    }

}

