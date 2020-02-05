#include "ResourceStateTracker.hpp"

#include "../Foundation/Assert.hpp"

namespace Memory
{

    void ResourceStateTracker::StartTrakingResource(const HAL::Resource* resource)
    {
        mCurrentResourceStates[resource] = resource->InitialStates();
    }

    void ResourceStateTracker::StopTrakingResource(const HAL::Resource* resource)
    {
        mCurrentResourceStates.erase(resource);
    }

    void ResourceStateTracker::RequestTransition(const HAL::Resource* resource, HAL::ResourceState newState)
    {
        mPendingResourceStates[resource] = newState;
    }

    HAL::ResourceBarrierCollection ResourceStateTracker::ApplyRequestedTransitions(bool firstInCommandList)
    {
        HAL::ResourceBarrierCollection barriers{};

        for (auto& [resource, state] : mPendingResourceStates)
        {
            auto barrier = TransitionToStateImmediately(resource, state, firstInCommandList);
            
            if (barrier)
            {
                barriers.AddBarrier(*barrier);
            }
        }

        mPendingResourceStates.clear();

        return barriers;
    }

    std::optional<HAL::ResourceTransitionBarrier> ResourceStateTracker::TransitionToStateImmediately(const HAL::Resource* resource, HAL::ResourceState newState, bool firstInCommandList)
    {
        ResourceStateIterator stateIt = GetCurrentStateForResource(resource);

        if (IsNewStateRedundant(stateIt->second, newState))
        {
            return std::nullopt;
        }

        if (CanTransitionToStateImplicitly(resource, stateIt->second, newState, firstInCommandList))
        {
            stateIt->second = newState;
            return std::nullopt;
        }

        HAL::ResourceTransitionBarrier barrier{ stateIt->second, newState, resource };
        stateIt->second = newState;
        return barrier;
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

