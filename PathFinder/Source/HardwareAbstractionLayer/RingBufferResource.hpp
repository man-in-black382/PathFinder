#pragma once

#include "Buffer.hpp"
#include "RingBuffer.hpp"

namespace HAL
{

    template <class T>
    class RingBufferResource : public Buffer<T>
    {
    public:
        RingBufferResource(
            const Device& device, 
            uint64_t elementCapacity,
            uint8_t frameCapacity,
            uint64_t perElementAlignment,
            CPUAccessibleHeapType heapType
        );

        virtual ~RingBufferResource() = default;

        // Read current frame memory
        virtual void Read(const Buffer<T>::ReadbackSession& session) const override;

        // Get virtual address of memory for current frame
        virtual D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress() const override;

        // Write to memory for current frame
        virtual void Write(uint64_t startIndex, const T* data, uint64_t dataLength = 1);

        // Get write-only pointer in current frame memory region
        virtual T* At(uint64_t index) override;

        uint64_t CurrentFrameByteOffset() const;
        uint64_t CurrentFrameObjectOffset() const;

        void PrepareMemoryForNewFrame(uint64_t newFrameFenceValue);
        void DiscardMemoryForCompletedFrames(uint64_t completedFrameNumber);

    private:
        RingBuffer mRingBuffer;
        RingBuffer::OffsetType mCurrentRingOffset = 0;
        uint64_t mPerFrameCapacity = 0;

    public:
        uint64_t PerFrameCapacity() const { return mPerFrameCapacity; }
    };

}

#include "RingBufferResource.inl"