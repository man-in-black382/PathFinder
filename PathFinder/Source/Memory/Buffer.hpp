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
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator, 
            PoolDescriptorAllocator* descriptorAllocator,
            CopyCommandListProvider* commandListProvider
        );

        template <class Element>
        Buffer(
            const HAL::Buffer::Properties<Element>& properties,
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyCommandListProvider* commandListProvider,
            const HAL::Device& device,
            const HAL::Heap& mainResourceExplicitHeap,
            uint64_t explicitHeapOffset
        );

        ~Buffer();

        template <class Element = uint8_t> 
        const HAL::SRDescriptor* GetOrCreateSRDescriptor(uint64_t elementAlignment = 1);

        template <class Element = uint8_t> 
        const HAL::UADescriptor* GetOrCreateUADescriptor(uint64_t elementAlignment = 1);

        template <class Element = uint8_t> 
        const HAL::CBDescriptor* GetOrCreateCBDescriptor(uint64_t elementAlignment = 1);

        void RequestWrite() override;
        void RequestRead() override;

        template <class Element = uint8_t>
        uint64_t ElementCapacity(uint64_t elementAlignment = 1) const;

        const HAL::Buffer* HALBuffer() const;
        const HAL::Resource* HALResource() const override;

    protected:
        virtual uint64_t ResourceSizeInBytes() const override;

    private:
        uint64_t mCurrentSRDescriptorStride = 1;
        uint64_t mCurrentUADescriptorStride = 1;
        uint64_t mCurrentCBDescriptorStride = 1;

        SegregatedPoolsResourceAllocator::BufferPtr mBufferPtr;

        PoolDescriptorAllocator::SRDescriptorPtr mSRDescriptor;
        PoolDescriptorAllocator::UADescriptorPtr mUADescriptor;
        PoolDescriptorAllocator::CBDescriptorPtr mCBDescriptor;
    };

}

#include "Buffer.inl"