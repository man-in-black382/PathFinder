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
            const Device& device, uint64_t capacity, ResourceState initialState,
            ResourceState expectedStates, HeapType heapType = HeapType::Default
        );

        ~BufferResource();

        void Write(uint64_t startIndex, const T* data, uint64_t dataLength = 1);

    private:
        uint64_t PaddedElementSize(ResourceState initialState);

        uint8_t* mMappedMemory = nullptr;
        uint64_t mPaddedElementSize = 0;
    };

    template <class T>
    uint64_t BufferResource<T>::PaddedElementSize(ResourceState initialState)
    {
        return BitwiseEnumMaskContainsComponent(initialState, ResourceState::ConstantBuffer) ? (elementSize + 255) & ~255 : elementSize;
    }

    template <class T>
    BufferResource<T>::BufferResource(const Device& device, uint64_t capacity, ResourceState initialState, ResourceState expectedStates, HeapType heapType)
        : Resource(device, ResourceFormat(std::nullopt, ResourceFormat::BufferKind::Buffer, Geometry::Dimensions{ PaddedElementSize(initialState) * capacity, 1, 1 }), initialState, expectedStates, heapType),
        mPaddedElementSize = PaddedElementSize(initialState);
    {
        if (heapType == Resource::HeapType::Upload || heapType == Resource::HeapType::Readback)
        {
            mResource->Map(0, nullptr, &mMappedMemory);
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
        memcpy(mMappedMemory + startIndex * mPaddedElementSize, data, sizeof(T) * dataLength);
    }

}

