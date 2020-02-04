#pragma once

#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceBarrier.hpp"

#include <unordered_map>

namespace PathFinder
{

    class ResourceStateTracker
    {
    public:
        void StartTrakingResource(const HAL::Resource* resource);
        void StopTrakingResource(const HAL::Resource* resource);

        // Queue state update but wait until CommitPendingTransitions
        void RequestTransition(const HAL::Resource* resource, HAL::ResourceState newState, bool firstInCommandList = false);

        // Register new resource states that are currently pending and return a corresponding barrier collection
        HAL::ResourceBarrierCollection ApplyRequestedTransitions();

        // Immediately record new state for a resource 
        HAL::ResourceTransitionBarrier TransitionToStateImmediately(
            const HAL::Resource* resource, HAL::ResourceState newState, bool firstInCommandList = false);

    private:
        using ResourceStateMap = std::unordered_map<const HAL::Resource*, HAL::ResourceState>;
        using ResourceStateIterator = ResourceStateMap::iterator;

        ResourceStateIterator GetCurrentStateForResource(const HAL::Resource* resource);
        bool IsNewStateRedundant(HAL::ResourceState currentState, HAL::ResourceState newState);
        bool CanTransitionToStateImplicitly(const HAL::Resource* resource, HAL::ResourceState currentState, HAL::ResourceState newState, bool firstInCommandList);

        ResourceStateMap mCurrentResourceStates;
        ResourceStateMap mPendingResourceStates;
    };

}
