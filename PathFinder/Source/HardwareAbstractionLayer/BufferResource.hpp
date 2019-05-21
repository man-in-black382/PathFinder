#pragma once

#include "Resource.hpp"
#include "Descriptor.hpp"

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

    protected:
        uint64_t PaddedElementSize(ResourceState initialState);

        uint8_t* mMappedMemory = nullptr;
        uint64_t mPaddedElementSize = 0;
        uint64_t mCapacity = 0;

    public:
        inline const auto Capacity() const { return mCapacity; }
    };

    template <class T>
    uint64_t BufferResource<T>::PaddedElementSize(ResourceState initialState)
    {
        return EnumMaskBitSet(initialState, ResourceState::ConstantBuffer) ? (sizeof(T) + 255) & ~255 : sizeof(T);
    }

    template <class T>
    BufferResource<T>::BufferResource(const Device& device, uint64_t capacity, ResourceState initialState, ResourceState expectedStates, HeapType heapType)
        : Resource(device, ResourceFormat(std::nullopt, ResourceFormat::BufferKind::Buffer, Geometry::Dimensions{ PaddedElementSize(initialState) * capacity }), initialState, expectedStates, heapType),
        mPaddedElementSize(PaddedElementSize(initialState)), mCapacity(capacity)
    {
        if (heapType == HeapType::Upload || heapType == HeapType::Readback)
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
        if (startIndex >= mCapacity) throw std::invalid_argument("Index is out of bounds");
        memcpy(mMappedMemory + startIndex * mPaddedElementSize, data, sizeof(T) * dataLength);
    }



    template <typename Vertex>
    class VertexBuffer : public BufferResource<Vertex>
    {
    public:
        VertexBuffer(const Device& device, uint64_t capacity, HeapType heapType = HeapType::Default);

    private:
        VertexBufferDescriptor mDescriptor;

    public:
        inline const auto& Descriptor() const { return mDescriptor; }
    };

    template <typename Vertex>
    VertexBuffer<Vertex>::VertexBuffer(const Device& device, uint64_t capacity, HeapType heapType)
        : BufferResource<Vertex>(device, capacity, ResourceState::VertexBuffer, ResourceState::VertexBuffer, heapType),
        mDescriptor(Resource::mResource->GetGPUVirtualAddress(), capacity * BufferResource<Vertex>::mPaddedElementSize, sizeof(Vertex)) {}



    template <typename Index>
    class IndexBuffer : public BufferResource<Index>
    {
    public:
        IndexBuffer(const Device& device, uint64_t capacity, ResourceFormat::Color format, HeapType heapType = HeapType::Default);

    private:
        IndexBufferDescriptor mDescriptor;

    public:
        inline const auto& Descriptor() const { return mDescriptor; }
    };

    template <typename Index>
    IndexBuffer<Index>::IndexBuffer(const Device& device, uint64_t capacity, ResourceFormat::Color format, HeapType heapType)
        : BufferResource(device, capacity, ResourceState::IndexBuffer, ResourceState::IndexBuffer, heapType),
        mDescriptor(Resource::mResource->GetGPUVirtualAddress(), capacity * BufferResource<Index>::mPaddedElementSize, ResourceFormat::D3DFormat(format)) {}

}

