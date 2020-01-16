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
        Buffer<T>(device, elementCapacity * frameCapacity, perElementAlignment, heapType),
        mRingBuffer{ this->Capacity() },
        mPerFrameCapacity{ elementCapacity } {}

    template <class T>
    void RingBufferResource<T>::Read(const Buffer<T>::ReadbackSession& session) const
    {
        // Read data for current frame
        Buffer<T>::Read(session, mCurrentRingOffset, mPerFrameCapacity);
    }

    template <class T>
    D3D12_GPU_VIRTUAL_ADDRESS RingBufferResource<T>::GPUVirtualAddress() const
    {
        return Buffer<T>::GPUVirtualAddress() + mCurrentRingOffset * Buffer<T>::PaddedElementSize();
    }

    template <class T>
    void RingBufferResource<T>::Write(uint64_t startIndex, const T* data, uint64_t dataLength)
    {
        Buffer<T>::Write(startIndex + mCurrentRingOffset, data, dataLength);
    }

    template <class T>
    T* RingBufferResource<T>::At(uint64_t index)
    {
        return Buffer<T>::At(index + mCurrentRingOffset);
    }

    template <class T>
    uint64_t RingBufferResource<T>::CurrentFrameByteOffset() const
    {
        return mCurrentRingOffset * Buffer<T>::PaddedElementSize();
    }

    template <class T>
    uint64_t RingBufferResource<T>::CurrentFrameObjectOffset() const
    {
        return mCurrentRingOffset;
    }

    template <class T>
    void RingBufferResource<T>::PrepareMemoryForNewFrame(uint64_t newFrameFenceValue)
    {
        mCurrentRingOffset = mRingBuffer.Allocate(mPerFrameCapacity);
        mRingBuffer.FinishCurrentFrame(newFrameFenceValue);
    }

    template <class T>
    void RingBufferResource<T>::DiscardMemoryForCompletedFrames(uint64_t completedFrameNumber)
    {
        mRingBuffer.ReleaseCompletedFrames(completedFrameNumber);
    }

}

