#pragma once

#include "GPUResource.hpp"

#include "../HardwareAbstractionLayer/Texture.hpp"

namespace Memory
{
   
    class Texture : public GPUResource
    {
    public:
        Texture(
            const HAL::Texture::Properties& properties, 
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyCommandListProvider* commandListProvider);

        Texture(
            const HAL::Texture::Properties& properties,
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyCommandListProvider* commandListProvider,
            const HAL::Device& device,
            const HAL::Heap& mainResourceExplicitHeap,
            uint64_t explicitHeapOffset);

        ~Texture();

        const HAL::RTDescriptor* GetOrCreateRTDescriptor();
        const HAL::DSDescriptor* GetOrCreateDSDescriptor();
        const HAL::SRDescriptor* GetOrCreateSRDescriptor();
        const HAL::UADescriptor* GetOrCreateUADescriptor();

        void RequestWrite() override;
        void RequestRead() override;

        const HAL::Texture* HALTexture() const;
        const HAL::Resource* HALResource() const override;

    protected:
        uint64_t ResourceSizeInBytes() const override;
        void ApplyDebugName() override;

    private:
        SegregatedPoolsResourceAllocator::TexturePtr mTexturePtr;
        PoolDescriptorAllocator::RTDescriptorPtr mRTDescriptor;
        PoolDescriptorAllocator::DSDescriptorPtr mDSDescriptor;
        PoolDescriptorAllocator::SRDescriptorPtr mSRDescriptor;
        PoolDescriptorAllocator::UADescriptorPtr mUADescriptor;
    };

}

