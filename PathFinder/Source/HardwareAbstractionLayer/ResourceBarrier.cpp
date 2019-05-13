#include "ResourceBarrier.hpp"
#include "Utils.h"
#include "Resource.hpp"

namespace HAL
{

    ResourceBarrier::~ResourceBarrier()
    {

    }

    ResourceTransitionBarrier::ResourceTransitionBarrier(D3D12_RESOURCE_STATES beforeStates, D3D12_RESOURCE_STATES afterStates, const Resource* resource)
        : mResource(resource)
    {
        mDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        mDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        mDesc.Transition.StateBefore = beforeStates;
        mDesc.Transition.StateAfter = afterStates;
        mDesc.Transition.pResource = resource->D3DPtr();
    }

}

