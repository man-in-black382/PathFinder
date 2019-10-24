#include "AsyncComputeDevice.hpp"

namespace PathFinder
{

    AsyncComputeDevice::AsyncComputeDevice(const HAL::Device* device, uint8_t simultaneousFramesInFlight)
        : mDevice{ device }, mRingCommandList{ *device, simultaneousFramesInFlight }, mCommandQueue{ *device } {}

    void AsyncComputeDevice::WaitFence(HAL::Fence& fence)
    {
        mCommandQueue.WaitFence(fence);
    }

    void AsyncComputeDevice::ExecuteCommands()
    {
        CommandList().Close();
        mCommandQueue.ExecuteCommandList(CommandList());
    }

    void AsyncComputeDevice::ResetCommandList()
    {
        mRingCommandList.CurrentCommandList().Reset(mRingCommandList.CurrentCommandAllocator());
    }

    void AsyncComputeDevice::SignalFence(HAL::Fence& fence)
    {
        mCommandQueue.SignalFence(fence);
    }

    void AsyncComputeDevice::BeginFrame(uint64_t frameFenceValue)
    {
        mRingCommandList.PrepareCommandListForNewFrame(frameFenceValue);
    }

    void AsyncComputeDevice::EndFrame(uint64_t completedFrameFenceValue)
    {
        mRingCommandList.ReleaseAndResetForCompletedFrames(completedFrameFenceValue);
    }

}
