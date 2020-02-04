#include "Buffer.hpp"

#include "../Foundation/MemoryUtils.hpp"

namespace HAL
{

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

    bool Buffer::CanImplicitlyPromoteFromCommonStateToState(ResourceState state) const
    {
        return true;
    }

    bool Buffer::CanImplicitlyDecayToCommonStateFromState(ResourceState state) const
    {
        return true;
    }

}

