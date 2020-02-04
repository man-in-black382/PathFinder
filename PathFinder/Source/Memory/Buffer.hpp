#pragma once

#include "GPUResource.hpp"

#include "../HardwareAbstractionLayer/Buffer.hpp"

namespace Memory
{
   
    class Buffer : public GPUResource
    {
    public:
        template <class Element>
        Buffer(
            const HAL::Buffer::Properties<Element>& properties,
            GPUResource::UploadStrategy uploadStrategy,
            SegregatedPoolsResourceAllocator* resourceAllocator, 
            HAL::CopyCommandListBase* commandList
        );

        ~Buffer() = default;

        void RequestWrite() override;
        void RequestRead() override;

        const HAL::Buffer* HALBuffer() const;
        const HAL::Resource* HALResource() const override;

    private:
        SegregatedPoolsResourceAllocator::BufferPtr mBufferPtr;
    };

}

#include "Buffer.inl"