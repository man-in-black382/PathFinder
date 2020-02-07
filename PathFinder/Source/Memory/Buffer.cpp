#include "Buffer.hpp"

namespace Memory
{

    Buffer::~Buffer()
    {
        if (StateTracker() && mBufferPtr) StateTracker()->StopTrakingResource(HALBuffer());
    }

    void Buffer::RequestWrite()
    {
        GPUResource::RequestWrite();
        CommandList()->CopyBufferRegion(*CurrentFrameUploadBuffer(), *HALBuffer(), 0, CurrentFrameUploadBuffer()->TotalMemory(), 0);
    }

    void Buffer::RequestRead()
    {
        GPUResource::RequestRead();
        CommandList()->CopyBufferRegion(*HALBuffer(), *CurrentFrameReadbackBuffer(), 0, HALBuffer()->TotalMemory(), 0);
    }

    const HAL::Buffer* Buffer::HALBuffer() const
    {
        return UploadStrategy() == GPUResource::UploadStrategy::Automatic ? 
            mBufferPtr.get() : CurrentFrameUploadBuffer();
    }

    const HAL::Resource* Buffer::HALResource() const
    {
        return HALBuffer();
    }

}
