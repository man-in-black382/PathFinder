#include "ResourceStateTracker.hpp"

#include "../Foundation/Assert.hpp"

namespace Memory
{

    void ResourceStateTracker::StartTrakingResource(const HAL::Resource* resource)
    {
        mCurrentResourceStates[resource].resize(resource->SubresourceCount(), resource->InitialStates());
    }

    void ResourceStateTracker::StopTrakingResource(const HAL::Resource* resource)
    {
        mCurrentResourceStates.erase(resource);
    }

    void ResourceStateTracker::RequestTransition(const HAL::Resource* resource, HAL::ResourceState newState)
    {
        ResourceStateTracker::SubresourceStateList& pendingStates = mPendingResourceStates[resource];
        pendingStates.resize(resource->SubresourceCount(), newState);
    }

    void ResourceStateTracker::RequestTransitions(const HAL::Resource* resource, const ResourceStateTracker::SubresourceStateList& newStates)
    {
        ResourceStateTracker::SubresourceStateList& pendingStates = mPendingResourceStates[resource];
        pendingStates = newStates;
    }

    HAL::ResourceBarrierCollection ResourceStateTracker::ApplyRequestedTransitions(bool firstInCommandList)
    {
        HAL::ResourceBarrierCollection barriers{};

        for (auto& [resource, subresourceStates] : mPendingResourceStates)
        {
            HAL::ResourceBarrierCollection resourceBarriers = TransitionToStatesImmediately(resource, subresourceStates, firstInCommandList);
            barriers.AddBarriers(resourceBarriers);
        }

        mPendingResourceStates.clear();

        return barriers;
    }

    HAL::ResourceBarrierCollection ResourceStateTracker::TransitionToStateImmediately(const HAL::Resource* resource, HAL::ResourceState newState, bool firstInCommandList)
    {
        SubresourceStateList& currentSubresourceStates = GetResourceCurrentStatesInternal(resource);
        HAL::ResourceBarrierCollection newStateBarriers{};
        HAL::ResourceState firstCurrentState = currentSubresourceStates.front();

        bool subresourceStatesMatch = true;

        for (auto subresource = 0u; subresource < currentSubresourceStates.size(); ++subresource)
        {
            HAL::ResourceState currentState = currentSubresourceStates[subresource];

            if (IsNewStateRedundant(currentState, newState))
            {
                continue;
            }

            currentSubresourceStates[subresource] = newState;

            if (CanTransitionToStateImplicitly(resource, currentState, newState, firstInCommandList))
            {
                continue;
            }

            newStateBarriers.AddBarrier(HAL::ResourceTransitionBarrier{ currentState, newState, resource, subresource });

            if (currentState != firstCurrentState)
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

    HAL::ResourceBarrierCollection ResourceStateTracker::TransitionToStatesImmediately(const HAL::Resource* resource, const SubresourceStateList& newStates, bool firstInCommandList)
    {
        SubresourceStateList& currentSubresourceStates = GetResourceCurrentStatesInternal(resource);

        assert_format(currentSubresourceStates.size() == newStates.size(), "Number of subresource states must match");

        HAL::ResourceBarrierCollection newStateBarriers{};

        for (auto subresource = 0u; subresource < currentSubresourceStates.size(); ++subresource)
        {
            HAL::ResourceState currentState = currentSubresourceStates[subresource];
            HAL::ResourceState newState = newStates[subresource];

            if (IsNewStateRedundant(currentState, newState))
            {
                continue;
            }

            currentSubresourceStates[subresource] = newState;

            if (CanTransitionToStateImplicitly(resource, currentState, newState, firstInCommandList))
            {
                continue;
            }

            newStateBarriers.AddBarrier(HAL::ResourceTransitionBarrier{ currentState, newState, resource, subresource });
        }

        return newStateBarriers;
    }

    const ResourceStateTracker::SubresourceStateList& ResourceStateTracker::ResourceCurrentStates(const HAL::Resource* resource) const
    {
        auto it = mCurrentResourceStates.find(resource);
        assert_format(it != mCurrentResourceStates.end(), "Resource is not registered / not being tracked");
        return it->second;
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
        // or current state is read state and new state is a partial or complete subset of the current 
        // (which implies that it is also a read state)
        return (currentState == newState) || (HAL::IsResourceStateReadOnly(currentState) && EnumMaskBitSet(currentState, newState));
    }

    bool ResourceStateTracker::CanTransitionToStateImplicitly(const HAL::Resource* resource, HAL::ResourceState currentState, HAL::ResourceState newState, bool firstInCommandList)
    {
        return firstInCommandList && resource->CanImplicitlyDecayToCommonStateFromState(currentState) && resource->CanImplicitlyPromoteFromCommonStateToState(newState);
    }

}

