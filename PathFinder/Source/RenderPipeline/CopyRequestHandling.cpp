#include "CopyRequestHandling.hpp"

namespace PathFinder
{

    void RecordCopyRequests(
        HAL::CopyCommandListBase& cmdList,
        Memory::ResourceStateTracker& stateTracker, 
        const std::vector<Memory::CopyRequestManager::CopyRequest>& requests,
        HAL::ResourceState copyState,
        bool applyBackTransition)
    {
        HAL::ResourceBarrierCollection preCopyTransisions{};
        HAL::ResourceBarrierCollection postCopyTransisions{};

        for (const Memory::CopyRequestManager::CopyRequest& copyRequest : requests)
        {
            const Memory::ResourceStateTracker::SubresourceStateList prevStates = stateTracker.ResourceCurrentStates(copyRequest.Resource);

            HAL::ResourceBarrierCollection barriers =
                stateTracker.TransitionToStateImmediately(copyRequest.Resource, copyState);

            preCopyTransisions.AddBarriers(barriers);
            
            // Return to previous state immediately after copy, if required
            if (applyBackTransition)
            {
                barriers = stateTracker.TransitionToStatesImmediately(copyRequest.Resource, prevStates);
                postCopyTransisions.AddBarriers(barriers);
            }
        }

        cmdList.InsertBarriers(preCopyTransisions);

        for (const Memory::CopyRequestManager::CopyRequest& copyRequest : requests)
        {
            copyRequest.Command(cmdList);
        }

        cmdList.InsertBarriers(postCopyTransisions);
    }

    void RecordUploadRequests(HAL::CopyCommandListBase& cmdList, Memory::ResourceStateTracker& stateTracker, Memory::CopyRequestManager& copyManager, bool applyBackTransition)
    {
        RecordCopyRequests(cmdList, stateTracker, copyManager.UploadRequests(), HAL::ResourceState::CopyDestination, applyBackTransition);
        copyManager.FlushUploadRequests();
    }

    void RecordReadbackRequests(HAL::CopyCommandListBase& cmdList, Memory::ResourceStateTracker& stateTracker, Memory::CopyRequestManager& copyManager, bool applyBackTransition)
    {
        RecordCopyRequests(cmdList, stateTracker, copyManager.ReadbackRequests(), HAL::ResourceState::CopySource, applyBackTransition);
        copyManager.FlushReadbackRequests();
    }

}
