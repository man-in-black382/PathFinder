#pragma once

namespace HAL
{
   
    template <class CommandListT, class CommandAllocatorT>
    RingCommandList<CommandListT, CommandAllocatorT>::RingCommandList(const Device& device, uint8_t frameCapacity) : mRingBuffer{ frameCapacity }
    {
        for (auto i = 0u; i < frameCapacity; i++)
        {
            mCommandAllocators.emplace_back(device);
            mCommandLists.emplace_back(device, mCommandAllocators.back());
        }

        mRingBuffer.SetDeallocationCallback([this](const RingBuffer::FrameTailAttributes& frameAttributes)
            {
                auto indexToReset = frameAttributes.Tail - frameAttributes.Size;
                mCommandAllocators[indexToReset].Reset();
                mCommandLists[indexToReset].Reset(mCommandAllocators[indexToReset]);
            });
    }

    template <class CommandListT, class CommandAllocatorT>
    void RingCommandList<CommandListT, CommandAllocatorT>::PrepareCommandListForNewFrame(uint64_t newFrameFenceValue)
    {
        mCurrentIndex = (uint8_t)mRingBuffer.Allocate(1);
        mRingBuffer.FinishCurrentFrame(newFrameFenceValue);
    }

    template <class CommandListT, class CommandAllocatorT>
    void RingCommandList<CommandListT, CommandAllocatorT>::ReleaseAndResetForCompletedFrames(uint64_t completedFrameFenceValue)
    {
        mRingBuffer.ReleaseCompletedFrames(completedFrameFenceValue);
    }

    template <class CommandListT, class CommandAllocatorT>
    CommandListT& RingCommandList<CommandListT, CommandAllocatorT>::CurrentCommandList()
    {
        return mCommandLists[mCurrentIndex];
    }

    template <class CommandListT, class CommandAllocatorT>
    CommandAllocatorT& RingCommandList<CommandListT, CommandAllocatorT>::CurrentCommandAllocator()
    {
        return mCommandAllocators[mCurrentIndex];
    }

}

