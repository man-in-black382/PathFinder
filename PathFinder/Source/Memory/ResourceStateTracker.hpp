#pragma once

#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceBarrier.hpp"

#include <unordered_map>

namespace Memory
{

    class ResourceStateTracker
    {
    public:
        struct SubresourceState
        {
            uint64_t SubresourceIndex = 0;
            HAL::ResourceState State = HAL::ResourceState::Common;
        };

        using SubresourceStateList = std::vector<SubresourceState>;

        void StartTrakingResource(const HAL::Resource* resource);
        void StopTrakingResource(const HAL::Resource* resource);

        // Queue state update but wait until ApplyRequestedTransitions
        void RequestTransition(const HAL::Resource* resource, HAL::ResourceState newState);
        void RequestTransitions(const HAL::Resource* resource, const SubresourceStateList& newStates);

        // Register new resource states that are currently pending and return a corresponding barrier collection
        HAL::ResourceBarrierCollection ApplyRequestedTransitions(bool firstInCommandList = false);

        // Immediately record new state for a resource 
        HAL::ResourceBarrierCollection TransitionToStateImmediately(const HAL::Resource* resource, HAL::ResourceState newState, bool firstInCommandList = false);
        HAL::ResourceBarrierCollection TransitionToStatesImmediately(const HAL::Resource* resource, const SubresourceStateList& newStates, bool firstInCommandList = false);

        const SubresourceStateList& ResourceCurrentStates(const HAL::Resource* resource) const;

    private:
        using ResourceStateMap = std::unordered_map<const HAL::Resource*, SubresourceStateList>;
        using ResourceStateIterator = ResourceStateMap::iterator;

        SubresourceStateList& GetResourceCurrentStatesInternal(const HAL::Resource* resource);

        bool IsNewStateRedundant(HAL::ResourceState currentState, HAL::ResourceState newState);
        bool CanTransitionToStateImplicitly(const HAL::Resource* resource, HAL::ResourceState currentState, HAL::ResourceState newState, bool firstInCommandList);

        ResourceStateMap mCurrentResourceStates;
        ResourceStateMap mPendingResourceStates;
    };

}
