#pragma once

#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/RingCommandList.hpp"
#include "../HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp"

namespace PathFinder
{

    class AsyncComputeDevice
    {
    public:
        AsyncComputeDevice(const HAL::Device* device, uint8_t simultaneousFramesInFlight);

        void WaitFence(HAL::Fence& fence);
        void ExecuteCommands();
        void ResetCommandList();
        void SignalFence(HAL::Fence& fence);

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

    private:
        const HAL::Device* mDevice;
        HAL::ComputeRingCommandList mRingCommandList;
        HAL::ComputeCommandQueue mCommandQueue;

    public:
        inline HAL::ComputeCommandList& CommandList() { return mRingCommandList.CurrentCommandList(); }
    };

}
