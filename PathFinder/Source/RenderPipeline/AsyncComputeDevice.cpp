#include "AsyncComputeDevice.hpp"

namespace PathFinder
{

    AsyncComputeDevice::AsyncComputeDevice(const HAL::Device* device, uint8_t simultaneousFramesInFlight)
        : mDevice{ device }, mRingCommandList{ *device, simultaneousFramesInFlight }, mCommandQueue{ *device } {}

    void AsyncComputeDevice::ExecuteCommandsThenSignalFence(HAL::Fence& fence)
    {

    }

    void AsyncComputeDevice::WaitFenceThenExecuteCommands(HAL::Fence& fence)
    {

    }

    void AsyncComputeDevice::BeginFrame(uint64_t frameFenceValue)
    {
        mRingCommandList.PrepareCommandListForNewFrame(frameFenceValue);
    }

    void AsyncComputeDevice::EndFrame(uint64_t completedFrameFenceValue)
    {
        mRingCommandList.ReleaseAndResetForCompletedFrames(completedFrameFenceValue);
    }

   /* void AsyncComputeDevice::ExecuteCommandsThenSignalFence(HAL::Fence& fence)
    {
        CommandList().Close();
        mCommandQueue.ExecuteCommandList(CommandList());
        mCommandQueue.SignalFence(fence);
    }*/


}
