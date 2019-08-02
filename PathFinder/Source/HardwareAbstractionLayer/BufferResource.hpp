#pragma once

#include "Resource.hpp"

namespace HAL
{

    template <class T>
    class BufferResource : public Resource
    {
    public:
        using Resource::Resource;

        BufferResource(
            const Device& device,
            uint64_t capacity,
            uint64_t perElementAlignment, 
            ResourceState initialState,
            ResourceState expectedStates,
            std::optional<CPUAccessibleHeapType> heapType = std::nullopt
        );

        ~BufferResource();

        virtual void Write(uint64_t startIndex, const T* data, uint64_t dataLength = 1);

    protected:
        uint64_t PaddedElementSize(uint64_t alignment);

        uint8_t* mMappedMemory = nullptr;
        uint64_t mPaddedElementSize = 0;
        uint64_t mCapacity = 0;

    public:
        inline const auto Capacity() const { return mCapacity; }
        inline const auto PaddedElementSize() const { return mPaddedElementSize; }
    };

    template <class T>
    uint64_t BufferResource<T>::PaddedElementSize(uint64_t alignment)
    {
        return (sizeof(T) + alignment - 1) & ~(alignment - 1);
    }

    template <class T>
    BufferResource<T>::BufferResource(
        const Device& device,
        uint64_t capacity, 
        uint64_t perElementAlignment,
        ResourceState initialState,
        ResourceState expectedStates,
        std::optional<CPUAccessibleHeapType> heapType)
        :
        Resource(
            device, 
            ResourceFormat(
                std::nullopt,
                ResourceFormat::BufferKind::Buffer,
                Geometry::Dimensions{ PaddedElementSize(perElementAlignment) * capacity }
            ),
            initialState,
            expectedStates,
            heapType
        ),
        mPaddedElementSize(PaddedElementSize(perElementAlignment)), mCapacity(capacity)
    {
        if (heapType)
        {
            ThrowIfFailed(mResource->Map(0, nullptr, (void**)&mMappedMemory));
        }
    }

    template <class T>
    BufferResource<T>::~BufferResource()
    {
        if (mMappedMemory)
        {
            mResource->Unmap(0, nullptr);
            mMappedMemory = nullptr;
        }
    }

    template <class T>
    void BufferResource<T>::Write(uint64_t startIndex, const T* data, uint64_t dataLength)
    {
        if (mPaddedElementSize > sizeof(T) && dataLength > 1) {
            throw std::invalid_argument(
                "Writing several objects into buffer that requires per object memory padding."
                "Instead of writing a continuous chunk of memory, write objects one by one in a loop."
            );
        }

        if (startIndex >= mCapacity) {
            throw std::invalid_argument("Index is out of bounds");
        }

        memcpy(mMappedMemory + startIndex * mPaddedElementSize, data, sizeof(T) * dataLength);
    }

}

