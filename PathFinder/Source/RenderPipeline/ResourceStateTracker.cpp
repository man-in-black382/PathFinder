#include "ResourceStateTracker.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    void ResourceStateTracker::StartTrakingResource(const HAL::Resource* resource)
    {
        mCurrentResourceStates[resource] = resource->InitialStates();
    }

    void ResourceStateTracker::StopTrakingResource(const HAL::Resource* resource)
    {
        mCurrentResourceStates.erase(resource);
    }

    void ResourceStateTracker::RequestTransition(const HAL::Resource* resource, HAL::ResourceState newState, bool firstInCommandList)
    {
        //HAL::IsResourceStateReadOnly(newState)
    }

    HAL::ResourceBarrierCollection ResourceStateTracker::ApplyRequestedTransitions()
    {

    }

    HAL::ResourceTransitionBarrier ResourceStateTracker::TransitionToStateImmediately(const HAL::Resource* resource, HAL::ResourceState newState, bool firstInCommandList)
    {

    }

    ResourceStateTracker::ResourceStateIterator ResourceStateTracker::GetCurrentStateForResource(const HAL::Resource* resource)
    {
        auto it = mCurrentResourceStates.find(resource);
        assert_format(it != mCurrentResourceStates.end(), "Resource is not registered / not being tracked");
        return it;
    }

    bool ResourceStateTracker::IsNewStateRedundant(HAL::ResourceState currentState, HAL::ResourceState newState)
    {
        // Transition is redundant if either states completely match 
        // or current state is read state and new state is a partial or complete subset of the current 
        // (which implies that it is also a read state)
        return (currentState == newState) || (HAL::IsResourceStateReadOnly(currentState) && EnumMaskBitSet(currentState, newState));
    }

    bool ResourceStateTracker::CanTransitionToStateImplicitly(const HAL::Resource* resource, HAL::ResourceState currentState, HAL::ResourceState newState, bool firstInCommandList)
    {
        return firstInCommandList && resource->CanImplicitlyDecayToCommonStateFromState(currentState) && resource->CanImplicitlyPromoteFromCommonStateToState(newState);
    }

}

