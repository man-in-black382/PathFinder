#pragma once

#include <Memory/ResourceStateTracker.hpp>
#include <Memory/CopyRequestManager.hpp>

namespace PathFinder
{

    void RecordUploadRequests(HAL::CopyCommandListBase& cmdList, Memory::ResourceStateTracker& stateTracker, Memory::CopyRequestManager& copyManager, bool applyBackTransition);
    void RecordReadbackRequests(HAL::CopyCommandListBase& cmdList, Memory::ResourceStateTracker& stateTracker, Memory::CopyRequestManager& copyManager, bool applyBackTransition);

}
