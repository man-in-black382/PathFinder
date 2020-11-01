#include "Buffer.hpp"

#include <Foundation/MemoryUtils.hpp>

namespace HAL
{

    Buffer::Buffer(const Device& device, const BufferProperties& properties, std::optional<CPUAccessibleHeapType> heapType)
        : Resource(device, ResourceFormat{ &device, properties }),
        mCPUAccessibleHeapType{ heapType },
        mProperties{ properties } {}

    Buffer::Buffer(const Device& device, const BufferProperties& properties, const Heap& heap, uint64_t heapOffset)
        : Resource(device, heap, heapOffset, ResourceFormat{ &device, properties }),
        mCPUAccessibleHeapType{ heap.CPUAccessibleType() },
        mProperties{ properties } {}

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

        D3D12_RANGE mapRange{ 0, mProperties.Size };
        ThrowIfFailed(mResource->Map(0, &mapRange, (void**)& mMappedMemory));
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

    bool Buffer::CanImplicitlyPromoteFromCommonStateToState(ResourceState state) const
    {
        return true;
    }

    bool Buffer::CanImplicitlyDecayToCommonStateFromState(ResourceState state) const
    {
        return true;
    }

}

