#pragma once

#include <cstdint>
#include <deque>

namespace HAL
{
   
    class RingBuffer
    {
    public:
        typedef size_t OffsetType;

        struct FrameTailAttributes
        {
            FrameTailAttributes(uint64_t fv, OffsetType off, OffsetType sz) 
                : FenceValue(fv), Offset(off), Size(sz) {}

            // Fence value associated with the command list in which
            // the allocation could have been referenced last time
            uint64_t FenceValue;
            OffsetType Offset;
            OffsetType Size;
        };

        static const OffsetType InvalidOffset = static_cast<OffsetType>(-1);

        RingBuffer(OffsetType maxSize);

        OffsetType Allocate(OffsetType Size);

        void FinishCurrentFrame(uint64_t fenceValue);
        void ReleaseCompletedFrames(uint64_t completedFenceValue);

        inline OffsetType MaxSize() const { return mMaxSize; }
        inline bool IsFull() const { return mUsedSize == mMaxSize; };
        inline bool IsEmpty() const { return mUsedSize == 0; };
        inline OffsetType UsedSize() const { return mUsedSize; }
        inline OffsetType CurrentFrameOffset() const { return mHead; }

    private:
        std::deque<FrameTailAttributes> mCompletedFrameTails;

        OffsetType mHead = 0;
        OffsetType mTail = 0;
        OffsetType mMaxSize = 0;
        OffsetType mUsedSize = 0;
        OffsetType mCurrentFrameSize = 0;
    };

}

