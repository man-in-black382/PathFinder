namespace HAL
{

    template <class T>
    RingBufferResource<T>::RingBufferResource(
        const Device& device,
        uint64_t elementCapacity,
        uint8_t frameCapacity,
        uint64_t perElementAlignment,
        CPUAccessibleHeapType heapType)
        :
        BufferResource<T>(device, elementCapacity * frameCapacity, perElementAlignment, heapType),
        mRingBuffer{ this->Capacity() },
        mPerFrameCapacity{ elementCapacity } {}

    template <class T>
    D3D12_GPU_VIRTUAL_ADDRESS RingBufferResource<T>::GPUVirtualAddress() const
    {
        return BufferResource<T>::GPUVirtualAddress() + mCurrentRingOffset * this->PaddedElementSize();
    }

    template <class T>
    void RingBufferResource<T>::Write(uint64_t startIndex, const T* data, uint64_t dataLength)
    {
        BufferResource<T>::Write(startIndex + mCurrentRingOffset, data, dataLength);
    }

    template <class T>
    T* RingBufferResource<T>::At(uint64_t index)
    {
        return BufferResource<T>::At(index + mCurrentRingOffset);
    }

    template <class T>
    void RingBufferResource<T>::PrepareMemoryForNewFrame(uint64_t newFrameFenceValue)
    {
        mCurrentRingOffset = mRingBuffer.Allocate(mPerFrameCapacity);
        mRingBuffer.FinishCurrentFrame(newFrameFenceValue);
    }

    template <class T>
    void RingBufferResource<T>::DiscardMemoryForCompletedFrames(uint64_t completedFrameFenceValue)
    {
        mRingBuffer.ReleaseCompletedFrames(completedFrameFenceValue);
    }

}

