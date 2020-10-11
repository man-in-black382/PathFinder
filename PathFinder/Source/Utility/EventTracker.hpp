#pragma once

#include <string>

#include <HardwareAbstractionLayer/CommandQueue.hpp>

namespace PathFinder
{

    class EventTracker
    {
    public:
        void StartGPUEvent(const std::string& eventName, const HAL::CommandList& commandList);
        void StartGPUEvent(const std::string& eventName, const HAL::CommandQueue& commandQueue);
        void SetMarker(const std::string& eventName, const HAL::CommandList& commandList);
        void SetMarker(const std::string& eventName, const HAL::CommandQueue& commandQueue);
        void EndGPUEvent(const HAL::CommandList& commandList);
        void EndGPUEvent(const  HAL::CommandQueue& commandQueue);
    };

}
