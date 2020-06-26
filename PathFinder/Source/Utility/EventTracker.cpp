#include "EventTracker.hpp"

#include "pix3.h"

namespace PathFinder
{

    void EventTracker::StartGPUEvent(const std::string& eventName, const HAL::CommandList& commandList)
    {
        PIXBeginEvent(commandList.D3DList(), PIX_COLOR_DEFAULT, "%s", eventName.c_str());
    }

    void EventTracker::StartGPUEvent(const std::string& eventName, const HAL::CommandQueue& commandQueue)
    {
        PIXBeginEvent(commandQueue.D3DQueue(), PIX_COLOR_DEFAULT, "%s", eventName.c_str());
    }

    void EventTracker::EndGPUEvent(const HAL::CommandList& commandList)
    {
        PIXEndEvent(commandList.D3DList());
    }

    void EventTracker::EndGPUEvent(const HAL::CommandQueue& commandQueue)
    {
        PIXEndEvent(commandQueue.D3DQueue());
    }

}
