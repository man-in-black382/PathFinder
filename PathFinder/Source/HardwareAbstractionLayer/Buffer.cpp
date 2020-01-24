#include "Buffer.hpp"

#include "../Foundation/MemoryUtils.hpp"

namespace HAL
{

    Buffer::Buffer(const Device& device, uint64_t size, ResourceState initialState, ResourceState expectedStates)
        : Resource(device, ConstructResourceFormat(&device, size), initialState, expectedStates),
        mSize{ size } {}

    Buffer::Buffer(const Device& device, const Heap& heap, uint64_t heapOffset, uint64_t size, ResourceState initialState, ResourceState expectedStates)
        : Resource(device, heap, heapOffset, ConstructResourceFormat(&device, size), initialState, expectedStates), 
        mSize{ size }, mCPUAccessibleHeapType{heap.CPUAccessibleType()} {}

    Buffer::Buffer(const Device& device, uint64_t size, CPUAccessibleHeapType heapType)
        : Resource(device, ConstructResourceFormat(&device, size), heapType),
        mSize{ size }, mCPUAccessibleHeapType{ heapType } {}

    Buffer::~Buffer()
    {
        Unmap();
    }

    uint8_t* Buffer::Map()
    {
        if (mMappedMemory)
        {
            return mMappedMemory;
        }

        ThrowIfFailed(mResource->Map(0, nullptr, (void**)& mMappedMemory));
        return mMappedMemory;
    }

    void Buffer::Unmap()
    {
        if (!mMappedMemory)
        {
            return;
        }

        mResource->Unmap(0, nullptr);
        mMappedMemory = nullptr;
    }

    uint32_t Buffer::SubresourceCount() const
    {
        return 1;
    }

    ResourceFormat Buffer::ConstructResourceFormat(const Device* device, uint64_t bufferSize)
    {
        return { 
            device, std::nullopt, BufferKind::Buffer, 
            Geometry::Dimensions{ bufferSize }
        };
    }

}

