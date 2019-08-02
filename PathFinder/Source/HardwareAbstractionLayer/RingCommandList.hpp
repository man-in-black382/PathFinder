#pragma once

#include "CommandList.hpp"
#include "RingBuffer.hpp"

#include <vector>

namespace HAL
{

    template <class CommandListT, class CommandAllocatorT>
    class RingCommandList
    {
    public:
        RingCommandList(const Device& device, uint8_t frameCapacity);

        void NewFrameStarted(uint64_t fenceValue);
        void FrameCompleted(uint64_t completedFenceValue);

        CommandListT& CurrentCommandList();
        CommandAllocatorT& CurrentCommandAllocator();

    private:
        std::vector<CommandAllocatorT> mCommandAllocators;
        std::vector<CommandListT> mCommandLists;
        RingBuffer mRingBuffer;
        uint8_t mCurrentIndex = 0;
    };

    template <class CommandListT, class CommandAllocatorT>
    HAL::RingCommandList<CommandListT, CommandAllocatorT>::RingCommandList(const Device& device, uint8_t frameCapacity) 
        : mRingBuffer{ frameCapacity }
    {
        for (auto i = 0u; i < frameCapacity; i++)
        {
            mCommandAllocators.emplace_back(device);
            mCommandLists.emplace_back(device, mCommandAllocators.back());
        }

        //for (auto i = 0u; i < frameCapacity; i++)
        //{
        //    //mCommandAllocators.emplace_back(device);
        //    mCommandLists.emplace_back(device, mCommandAllocators[i]);
        //}
    }

    template <class CommandListT, class CommandAllocatorT>
    void HAL::RingCommandList<CommandListT, CommandAllocatorT>::NewFrameStarted(uint64_t fenceValue)
    {
        mCurrentIndex = mRingBuffer.Allocate(1);
        mRingBuffer.FinishCurrentFrame(fenceValue);
    }

    template <class CommandListT, class CommandAllocatorT>
    void HAL::RingCommandList<CommandListT, CommandAllocatorT>::FrameCompleted(uint64_t completedFenceValue)
    {
        mRingBuffer.ReleaseCompletedFrames(completedFenceValue);
    }

    template <class CommandListT, class CommandAllocatorT>
    CommandListT& HAL::RingCommandList<CommandListT, CommandAllocatorT>::CurrentCommandList()
    {
        return mCommandLists[mCurrentIndex];
    }

    template <class CommandListT, class CommandAllocatorT>
    CommandAllocatorT& HAL::RingCommandList<CommandListT, CommandAllocatorT>::CurrentCommandAllocator()
    {
        return mCommandAllocators[mCurrentIndex];
    }



    class DirectRingCommandList : public RingCommandList<DirectCommandList, DirectCommandAllocator> 
    {
    public:
        using RingCommandList<DirectCommandList, DirectCommandAllocator>::RingCommandList;
    };

    class ComputeRingCommandList : public RingCommandList<ComputeCommandList, ComputeCommandAllocator>
    {
    public:
        using RingCommandList<ComputeCommandList, ComputeCommandAllocator>::RingCommandList;
    };

    class CopyRingCommandList : public RingCommandList<CopyCommandList, CopyCommandAllocator>
    {
    public:
        using RingCommandList<CopyCommandList, CopyCommandAllocator>::RingCommandList;
    };

}

