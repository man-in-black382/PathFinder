#pragma once

#include "GPUResource.hpp"

#include <HardwareAbstractionLayer/Buffer.hpp>

namespace Memory
{

    class Buffer : public GPUResource
    {
    public:
        Buffer(
            const HAL::BufferProperties& properties,
            GPUResource::AccessStrategy accessStrategy,
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator, 
            PoolDescriptorAllocator* descriptorAllocator,
            CopyRequestManager* copyRequestManager
        );

        Buffer(
            const HAL::BufferProperties& properties,
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyRequestManager* copyRequestManager,
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

        void BeginFrame(uint64_t frameNumber) override;

    protected:
        uint64_t ResourceSizeInBytes() const override;
        void ApplyDebugName() override;
        CopyRequestManager::CopyCommand GetUploadCommands() override;
        CopyRequestManager::CopyCommand GetReadbackCommands() override;

    private:
        uint64_t mRequstedStride = 1;
        HAL::BufferProperties mProperties;

        SegregatedPoolsResourceAllocator::BufferPtr mBufferPtr;
        HAL::Buffer* mGetterBufferPtr = nullptr;

        // Cached values, to be mutated from getters
        mutable uint64_t mCBDescriptorRequestFrameNumber = 0;
        mutable uint64_t mSRDescriptorRequestFrameNumber = 0;
        mutable PoolDescriptorAllocator::SRDescriptorPtr mSRDescriptor;
        mutable PoolDescriptorAllocator::UADescriptorPtr mUADescriptor;
        mutable PoolDescriptorAllocator::CBDescriptorPtr mCBDescriptor;

    public:
        inline const auto& Properties() const { return mProperties; }
    };

}

#include "Buffer.inl"