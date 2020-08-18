#include "ResourceStateTracker.hpp"

#include "../Foundation/Assert.hpp"

namespace Memory
{

    void ResourceStateTracker::StartTrakingResource(const HAL::Resource* resource)
    {
        SubresourceStateList& currentStates = mCurrentResourceStates[resource];
        currentStates.resize(resource->SubresourceCount(), { 0, resource->InitialStates() });

        for (auto subresourceIdx = 0u; subresourceIdx < resource->SubresourceCount(); ++subresourceIdx)
        {
            currentStates[subresourceIdx].SubresourceIndex = subresourceIdx;
        }
    }

    void ResourceStateTracker::StopTrakingResource(const HAL::Resource* resource)
    {
        mCurrentResourceStates.erase(resource);
    }

    void ResourceStateTracker::RequestTransition(const HAL::Resource* resource, HAL::ResourceState newState)
    {
        SubresourceStateList& pendingStates = mPendingResourceStates[resource];
        pendingStates.resize(resource->SubresourceCount(), { 0, newState });

        for (auto subresourceIdx = 0u; subresourceIdx < resource->SubresourceCount(); ++subresourceIdx)
        {
            pendingStates[subresourceIdx].SubresourceIndex = subresourceIdx;
        }
    }

    void ResourceStateTracker::RequestTransitions(const HAL::Resource* resource, const ResourceStateTracker::SubresourceStateList& newStates)
    {
        SubresourceStateList& pendingStates = mPendingResourceStates[resource];
        pendingStates.insert(pendingStates.end(), newStates.begin(), newStates.end());
    }

    HAL::ResourceBarrierCollection ResourceStateTracker::ApplyRequestedTransitions(bool tryApplyImplicitly)
    {
        HAL::ResourceBarrierCollection barriers{};

        for (auto& [resource, subresourceStates] : mPendingResourceStates)
        {
            HAL::ResourceBarrierCollection resourceBarriers = TransitionToStatesImmediately(resource, subresourceStates, tryApplyImplicitly);
            barriers.AddBarriers(resourceBarriers);
        }

        mPendingResourceStates.clear();

        return barriers;
    }

    HAL::ResourceBarrierCollection ResourceStateTracker::TransitionToStateImmediately(const HAL::Resource* resource, HAL::ResourceState newState, bool tryApplyImplicitly)
    {
        SubresourceStateList& currentSubresourceStates = GetResourceCurrentStatesInternal(resource);
        HAL::ResourceBarrierCollection newStateBarriers{};
        HAL::ResourceState firstCurrentState = currentSubresourceStates.front().State;

        bool subresourceStatesMatch = true;

        for (SubresourceState& subresourceState : currentSubresourceStates)
        {
            HAL::ResourceState oldState = subresourceState.State;

            if (IsNewStateRedundant(oldState, newState))
            {
                continue;
            }

            subresourceState.State = newState;

            if (CanTransitionToStateImplicitly(resource, oldState, newState, tryApplyImplicitly))
            {
                continue;
            }

            newStateBarriers.AddBarrier(HAL::ResourceTransitionBarrier{ oldState, newState, resource, subresourceState.SubresourceIndex });

            if (oldState != firstCurrentState)
            {
                subresourceStatesMatch = false;
            }
        }

        // If multiple transitions were requested, but it's possible to make just one - do it
        if (subresourceStatesMatch && newStateBarriers.BarrierCount() > 1)
        {
            HAL::ResourceBarrierCollection singleBarrierCollection{};
            singleBarrierCollection.AddBarrier(HAL::ResourceTransitionBarrier{ firstCurrentState, newState, resource });
            return singleBarrierCollection;
        }

        return newStateBarriers;
    }

    std::optional<HAL::ResourceTransitionBarrier> ResourceStateTracker::TransitionToStateImmediately(const HAL::Resource* resource, HAL::ResourceState newState, uint64_t subresourceIndex, bool tryApplyImplicitly)
    {
        SubresourceStateList& currentSubresourceStates = GetResourceCurrentStatesInternal(resource);
        assert_format(subresourceIndex < currentSubresourceStates.size(), "Requested a state change for subresource that doesn't exist");
        HAL::ResourceState oldState = currentSubresourceStates[subresourceIndex].State;

        if (IsNewStateRedundant(oldState, newState))
        {
            return std::nullopt;
        }

        currentSubresourceStates[subresourceIndex].State = newState;

        if (CanTransitionToStateImplicitly(resource, oldState, newState, tryApplyImplicitly))
        {
            return std::nullopt;
        }

        return HAL::ResourceTransitionBarrier{ oldState, newState, resource, subresourceIndex };
    }

    HAL::ResourceBarrierCollection ResourceStateTracker::TransitionToStatesImmediately(const HAL::Resource* resource, const SubresourceStateList& newStates, bool tryApplyImplicitly)
    {
        SubresourceStateList& currentSubresourceStates = GetResourceCurrentStatesInternal(resource);

        HAL::ResourceBarrierCollection newStateBarriers{};

        bool statesMatch = true;

        HAL::ResourceState firstOldState = currentSubresourceStates.front().State;
        HAL::ResourceState firstNewState = newStates.front().State;

        for (const SubresourceState& newSubresourceState : newStates)
        {
            assert_format(newSubresourceState.SubresourceIndex < currentSubresourceStates.size(), "Requested a state change for subresource that doesn't exist");

            SubresourceState& currentState = currentSubresourceStates[newSubresourceState.SubresourceIndex];
            HAL::ResourceState oldState = currentState.State;
            HAL::ResourceState newState = newSubresourceState.State;

            if (IsNewStateRedundant(oldState, newState))
            {
                continue;
            }

            currentState.State = newSubresourceState.State;

            if (CanTransitionToStateImplicitly(resource, oldState, newState, tryApplyImplicitly))
            {
                continue;
            }

            newStateBarriers.AddBarrier(HAL::ResourceTransitionBarrier{ oldState, newState, resource, newSubresourceState.SubresourceIndex });

            // If any old subresource states do not match or any of the new states do not match
            // then performing single transition barrier for all subresources is not possible
            if (oldState != firstOldState || newState != firstNewState)
            {
                statesMatch = false;
            }
        }

        // If multiple transitions were requested, but it's possible to make just one - do it
        if (statesMatch && newStateBarriers.BarrierCount() > 1)
        {
            HAL::ResourceBarrierCollection singleBarrierCollection{};
            singleBarrierCollection.AddBarrier(HAL::ResourceTransitionBarrier{ firstOldState, firstNewState, resource });
            return singleBarrierCollection;
        }

        return newStateBarriers;
    }

    const ResourceStateTracker::SubresourceStateList& ResourceStateTracker::ResourceCurrentStates(const HAL::Resource* resource) const
    {
        auto it = mCurrentResourceStates.find(resource);
        assert_format(it != mCurrentResourceStates.end(), "Resource is not registered / not being tracked");
        return it->second;
    }

    bool ResourceStateTracker::CanResourceBeImplicitlyTransitioned(const HAL::Resource& resource, HAL::ResourceState fromState, HAL::ResourceState toState)
    {
        return resource.CanImplicitlyDecayToCommonStateFromState(fromState) && resource.CanImplicitlyPromoteFromCommonStateToState(toState);
    }

    ResourceStateTracker::SubresourceStateList& ResourceStateTracker::GetResourceCurrentStatesInternal(const HAL::Resource* resource)
    {
        auto it = mCurrentResourceStates.find(resource);
        assert_format(it != mCurrentResourceStates.end(), "Resource is not registered / not being tracked");
        return it->second;
    }

    bool ResourceStateTracker::IsNewStateRedundant(HAL::ResourceState currentState, HAL::ResourceState newState)
    {
        // Transition is redundant if either states completely match 
        // or current state is a read state and new state is a partial or complete subset of the current 
        // (which implies that it is also a read state)
        return (currentState == newState) || (HAL::IsResourceStateReadOnly(currentState) && EnumMaskEquals(currentState, newState));
    }

    bool ResourceStateTracker::CanTransitionToStateImplicitly(const HAL::Resource* resource, HAL::ResourceState currentState, HAL::ResourceState newState, bool tryApplyImplicitly)
    {
        return tryApplyImplicitly && CanResourceBeImplicitlyTransitioned(*resource, currentState, newState);
    }

}

