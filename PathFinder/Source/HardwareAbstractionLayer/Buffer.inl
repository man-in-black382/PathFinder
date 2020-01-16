#pragma once

#include "../Foundation/MemoryUtils.hpp"

namespace HAL
{
    template <class T>
    Buffer<T>::Buffer(const Device& device, uint64_t capacity, uint64_t perElementAlignment, CPUAccessibleHeapType heapType)
        :
        Resource(device, ConstructResourceFormat(&device, capacity, perElementAlignment), heapType),
        mNonPaddedElementSize{ sizeof(T) },
        mPaddedElementSize{ PaddedElementSize(perElementAlignment) },
        mCapacity{ capacity },
        mPerElementAlignment{ perElementAlignment }
    {
        // Upload heaps can be mapped persistently
        if (heapType == CPUAccessibleHeapType::Upload)
        {
            ThrowIfFailed(mResource->Map(0, nullptr, (void**)& mMappedMemory));
        }
    }

    template <class T>
    Buffer<T>::Buffer(
        const Device& device, uint64_t capacity, uint64_t perElementAlignment, 
        ResourceState initialState, ResourceState expectedStates)
        : 
        Resource(device, ConstructResourceFormat(&device, capacity, perElementAlignment), initialState, expectedStates),
        mNonPaddedElementSize{ sizeof(T) },
        mPaddedElementSize{ PaddedElementSize(perElementAlignment) },
        mCapacity{ capacity },
        mPerElementAlignment{ perElementAlignment }
    { }

    template <class T>
    HAL::Buffer<T>::Buffer(
        const Device& device, const Heap& heap, uint64_t heapOffset, uint64_t capacity,
        uint64_t perElementAlignment, ResourceState initialState, ResourceState expectedStates)
        :
        Resource(device, heap, heapOffset, ConstructResourceFormat(&device, capacity, perElementAlignment), initialState, expectedStates),
        mNonPaddedElementSize{ sizeof(T) },
        mPaddedElementSize{ PaddedElementSize(perElementAlignment) },
        mCapacity{ capacity },
        mPerElementAlignment{ perElementAlignment }
    {}

    template <class T>
    Buffer<T>::~Buffer()
    {
        if (mMappedMemory)
        {
            mResource->Unmap(0, nullptr);
            mMappedMemory = nullptr;
        }
    }

    template <class T>
    void HAL::Buffer<T>::ValidateMappedMemory() const
    {
        if (!mMappedMemory)
        {
            throw std::runtime_error("Buffer resource is not readable by CPU");
        }
    }

    template <class T>
    void HAL::Buffer<T>::ValidateIndex(uint64_t index) const
    {
        if (index >= mCapacity) {
            throw std::invalid_argument("Index is out of bounds");
        }
    }

    template <class T>
    uint64_t Buffer<T>::PaddedElementSize(uint64_t alignment)
    {
        return (sizeof(T) + alignment - 1) & ~(alignment - 1);
    }

    template <class T>
    void Buffer<T>::Read(const ReadbackSession& session, uint64_t startOffset, uint64_t objectCount) const
    {
        const T* mappedMemory = nullptr;
        D3D12_RANGE readRange{ startOffset * mPaddedElementSize, (startOffset + objectCount) * mPaddedElementSize };
        ThrowIfFailed(mResource->Map(0, &readRange, (void**)& mappedMemory));
        session(mappedMemory);
        mResource->Unmap(0, nullptr);
    }

    template <class T>
    void Buffer<T>::Read(const ReadbackSession& session) const
    {
        Read(session, 0, mCapacity);
    }

    template <class T>
    void Buffer<T>::Write(uint64_t startIndex, const T* data, uint64_t dataLength)
    {
        ValidateMappedMemory();
        ValidateIndex(startIndex);

        if (mPaddedElementSize > sizeof(T) && dataLength > 1)
        {
            throw std::invalid_argument(
                "Writing several objects into buffer that requires per object memory padding."
                "Instead of writing a continuous chunk of memory, write objects one by one in a loop."
            );
        }

        memcpy(mMappedMemory + startIndex * mPaddedElementSize, data, sizeof(T) * dataLength);
    }

    template <class T>
    T* HAL::Buffer<T>::At(uint64_t index) 
    {
        ValidateMappedMemory();
        ValidateIndex(index);

        return reinterpret_cast<T*>(mMappedMemory + index * mPaddedElementSize);
    }

    template <class T>
    uint32_t Buffer<T>::SubresourceCount() const
    {
        return 1;
    }

    template <class T>
    ResourceFormat Buffer<T>::ConstructResourceFormat(const Device* device, uint64_t capacity, uint64_t perElementAlignment)
    {
        return { 
            device, std::nullopt, BufferKind::Buffer, 
            Geometry::Dimensions{ Foundation::MemoryUtils::Align(sizeof(T), perElementAlignment) * capacity }
        };
    }

}

