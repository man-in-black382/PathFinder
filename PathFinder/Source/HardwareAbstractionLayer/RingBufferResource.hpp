#pragma once

#include "RingBuffer.hpp"
#include "BufferResource.hpp"

namespace HAL
{

    template <class T>
    class RingBufferResource : protected BufferResource<T>
    {
    public:
        RingBufferResource(
            const Device& device, 
            uint64_t elementCapacity,
            uint8_t frameCapacity,
            uint64_t perElementAlignment,
            ResourceState initialState,
            ResourceState expectedStates,
            CPUAccessibleHeapType heapType
        );

        virtual ~RingBufferResource() = default;

        virtual D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress() const override;

        virtual void Write(uint64_t startIndex, const T* data, uint64_t dataLength = 1);

        void NewFrameStarted(uint64_t fenceValue);
        void FrameCompleted(uint64_t completedFenceValue);

    private:
        RingBuffer mRingBuffer;
        RingBuffer::OffsetType mCurrentRingOffset = 0;
        uint64_t mElementCapacity = 0;
    };

    template <class T>
    RingBufferResource<T>::RingBufferResource(
        const Device& device,
        uint64_t elementCapacity, 
        uint8_t frameCapacity,
        uint64_t perElementAlignment,
        ResourceState initialState,
        ResourceState expectedStates,
        CPUAccessibleHeapType heapType)
        :
        BufferResource<T>(device, elementCapacity * frameCapacity, perElementAlignment, initialState, expectedStates, heapType),
        mRingBuffer{ this->Capacity() },
        mElementCapacity{ elementCapacity } {}

    template <class T>
    D3D12_GPU_VIRTUAL_ADDRESS RingBufferResource<T>::GPUVirtualAddress() const
    {
        return this->GPUVirtualAddress() + mCurrentRingOffset * this->PaddedElementSize();
    }

    template <class T>
    void RingBufferResource<T>::Write(uint64_t startIndex, const T* data, uint64_t dataLength)
    {
        BufferResource<T>::Write(startIndex + mCurrentRingOffset, data, dataLength);
    }

    template <class T>
    void HAL::RingBufferResource<T>::NewFrameStarted(uint64_t fenceValue)
    {
        mCurrentRingOffset = mRingBuffer.Allocate(mElementCapacity);
        mRingBuffer.FinishCurrentFrame(fenceValue);
    }

    template <class T>
    void HAL::RingBufferResource<T>::FrameCompleted(uint64_t completedFenceValue)
    {
        mRingBuffer.ReleaseCompletedFrames(completedFenceValue);
    }

}

