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

        void PrepareCommandListForNewFrame(uint64_t newFrameFenceValue);
        void ReleaseAndResetForCompletedFrames(uint64_t completedFrameFenceValue);

        CommandListT& CurrentCommandList();
        CommandAllocatorT& CurrentCommandAllocator();

    private:
        std::vector<CommandAllocatorT> mCommandAllocators;
        std::vector<CommandListT> mCommandLists;
        RingBuffer mRingBuffer;
        uint8_t mCurrentIndex = 0;
    };



    class GraphicsRingCommandList : public RingCommandList<GraphicsCommandList, GraphicsCommandAllocator> 
    {
    public:
        using RingCommandList<GraphicsCommandList, GraphicsCommandAllocator>::RingCommandList;
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

#include "RingCommandList.inl"
