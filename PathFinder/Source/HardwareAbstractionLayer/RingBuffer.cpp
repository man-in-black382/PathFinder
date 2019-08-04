#include "RingBuffer.hpp"

namespace HAL
{

    RingBuffer::RingBuffer(OffsetType maxSize)
        : mMaxSize{ maxSize } {}

    RingBuffer::OffsetType RingBuffer::Allocate(OffsetType size)
    {
        if (IsFull())
        {
            return InvalidOffset;
        }

        if (mTail >= mHead)
        {
            //                     Head             Tail     MaxSize
            //                     |                |        |
            //  [                  xxxxxxxxxxxxxxxxx         ]
            //                                        
            //
            if (mTail + size <= mMaxSize)
            {
                auto offset = mTail;
                mTail += size;
                mUsedSize += size;
                mCurrentFrameSize += size;
                return offset;
            }
            else if (size <= mHead)
            {
                // Allocate from the beginning of the buffer
                OffsetType addSize = (mMaxSize - mTail) + size;
                mUsedSize += addSize;
                mCurrentFrameSize += addSize;
                mTail = size;
                return 0;
            }
        }
        else if (mTail + size <= mHead)
        {
            //
            //       Tail          Head            
            //       |             |            
            //  [xxxx              xxxxxxxxxxxxxxxxxxxxxxxxxx]
            //
            auto offset = mTail;
            mTail += size;
            mUsedSize += size;
            mCurrentFrameSize += size;
            return offset;
        }

        return InvalidOffset;
    }

    void RingBuffer::FinishCurrentFrame(uint64_t fenceValue)
    {
        mCompletedFrameTails.emplace_back(fenceValue, mTail, mCurrentFrameSize);
        mCurrentFrameSize = 0;
    }

    void RingBuffer::ReleaseCompletedFrames(uint64_t completedFenceValue)
    {
        // We can release all tails whose associated fence value is less 
        // than or equal to CompletedFenceValue
        while (!mCompletedFrameTails.empty() &&
            mCompletedFrameTails.front().FenceValue <= completedFenceValue)
        {
            const auto &oldestFrameTail = mCompletedFrameTails.front();

            if (oldestFrameTail.Size > mUsedSize) {
                throw std::runtime_error("Oldest frame's tail is bigger than total used size");
            }

            mUsedSize -= oldestFrameTail.Size;
            mHead = oldestFrameTail.Tail;
            mDeallocationCallback(oldestFrameTail);
            mCompletedFrameTails.pop_front();
        }
    }

    void RingBuffer::SetDeallocationCallback(const DeallocationCallback& callback)
    {
        mDeallocationCallback = callback;
    }

}
