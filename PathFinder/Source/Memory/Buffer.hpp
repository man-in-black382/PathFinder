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
            const HAL::BufferProperties<Element>& properties,
            GPUResource::UploadStrategy uploadStrategy,
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator, 
            PoolDescriptorAllocator* descriptorAllocator,
            CopyCommandListProvider* commandListProvider
        );

        template <class Element>
        Buffer(
            const HAL::BufferProperties<Element>& properties,
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyCommandListProvider* commandListProvider,
            const HAL::Device& device,
            const HAL::Heap& mainResourceExplicitHeap,
            uint64_t explicitHeapOffset
        );

        ~Buffer();

        const HAL::SRDescriptor* GetSRDescriptor() const;
        const HAL::UADescriptor* GetUADescriptor() const;
        const HAL::CBDescriptor* GetCBDescriptor() const;

        template <class Element = uint8_t>
        uint64_t Capacity(uint64_t elementAlignment = 1) const;

        const HAL::Buffer* HALBuffer() const;
        const HAL::Resource* HALResource() const override;

    protected:
        uint64_t ResourceSizeInBytes() const override;
        void ApplyDebugName() override;
        void RecordUploadCommands() override;
        void RecordReadbackCommands() override;

    private:
        uint64_t mRequstedStride = 1;

        SegregatedPoolsResourceAllocator::BufferPtr mBufferPtr;

        // Cached values, to be mutated from getters
        mutable uint64_t mCBDescriptorRequestFrameNumber = 0;
        mutable uint64_t mSRDescriptorRequestFrameNumber = 0;
        mutable PoolDescriptorAllocator::SRDescriptorPtr mSRDescriptor;
        mutable PoolDescriptorAllocator::UADescriptorPtr mUADescriptor;
        mutable PoolDescriptorAllocator::CBDescriptorPtr mCBDescriptor;
    };

}

#include "Buffer.inl"