#include "../Foundation/MemoryUtils.hpp"
#include "../Foundation/Assert.hpp"

namespace HAL
{

    template <class T>
    BufferCPUAccessor<T>::BufferCPUAccessor(Buffer* buffer, uint64_t elementCapacity, uint64_t elementPadding)
        : mBuffer{ buffer }, mCapacity{ elementCapacity }, mElementPadding{ elementPadding },
        mNonPaddedElementSize{ sizeof(T) }, mPaddedElementSize{ Foundation::MemoryUtils::Align(sizeof(T), elementPadding) }
    {
        assert_format(mCapacity* mPaddedElementSize <= mBuffer.Size(), "Requested memory to access exceeds buffer's total size.");
        assert_format(mBuffer->HeapType(), "Buffer is not accessible by CPU. It was created on a default heap.");
    }

    template <class T>
    BufferCPUAccessor<T>::~BufferCPUAccessor()
    {
        mDeallocationCallback();
    }

    template <class T>
    void BufferCPUAccessor<T>::Read(const ReadbackSession& session) const
    {
        assert_format(mBuffer->HeapType() == CPUAccessibleHeapType::Readback, "Buffer is not readable by CPU");

        const T* mappedMemory = mBuffer->Map();
        session(mappedMemory);
        mResource->Unmap(); // Invalidate CPU cache before next read
    }

    template <class T>
    void BufferCPUAccessor<T>::Write(uint64_t startIndex, const T* data, uint64_t dataLength)
    {
        assert_format(mBuffer->HeapType() == CPUAccessibleHeapType::Upload, "Buffer is not writable by CPU");
        assert_format(index < mCapacity, "Index is out of bounds");
        assert_format(mPaddedElementSize == sizeof(T) || dataLength <= 1,
            "Writing several objects into buffer that requires per object memory padding."
            "Instead of writing a continuous chunk of memory, write objects one by one in a loop.");

        T* memory = mBuffer->Map();
        memcpy(memory + (mGlobalObjectOffset + startIndex) * mPaddedElementSize, data, sizeof(T) * dataLength);
    }

    template <class T>
    T* BufferCPUAccessor<T>::GetWriteOnlyPointer(uint64_t index)
    {
        assert_format(mBuffer->HeapType() == CPUAccessibleHeapType::Upload, "Buffer is not writable by CPU");
        assert_format(index < mCapacity, "Index is out of bounds");

        T* memory = mBuffer->Map();
        return reinterpret_cast<T*>(memory + (index + mGlobalObjectOffset) * mPaddedElementSize);
    }

    template <class T>
    void BufferCPUAccessor<T>::SetGlobalOffsetInObjects(uint64_t offset)
    {
        mGlobalObjectOffset = offset;
    }

    template <class T>
    void BufferCPUAccessor<T>::SetDeallocationCallback(const DeallocationCallback& callback)
    {
        mDeallocationCallback = callback;
    }

}

