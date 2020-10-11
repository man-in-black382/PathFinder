#pragma once

#include "GPUResource.hpp"

#include <HardwareAbstractionLayer/Texture.hpp>

#include <vector>

namespace Memory
{
   
    class Texture : public GPUResource
    {
    public:
        Texture(
            const HAL::TextureProperties& properties, 
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyRequestManager* copyRequestManager);

        Texture(
            const HAL::TextureProperties& properties,
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyRequestManager* copyRequestManager,
            const HAL::Device& device,
            const HAL::Heap& mainResourceExplicitHeap,
            uint64_t explicitHeapOffset);

        Texture(
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyRequestManager* copyRequestManager,
            HAL::Texture* existingTexture);

        ~Texture();

        const HAL::RTDescriptor* GetRTDescriptor(uint8_t mipLevel = 0) const;
        const HAL::DSDescriptor* GetDSDescriptor() const;
        const HAL::SRDescriptor* GetSRDescriptor() const;
        const HAL::UADescriptor* GetUADescriptor(uint8_t mipLevel = 0) const;

        const HAL::Texture* HALTexture() const;
        const HAL::Resource* HALResource() const override;

    protected:
        uint64_t ResourceSizeInBytes() const override;
        void ApplyDebugName() override;
        CopyRequestManager::CopyCommand GetUploadCommands() override;
        CopyRequestManager::CopyCommand GetReadbackCommands() override;
        void ReserveDiscriptorArrays(uint8_t mipCount);

    private:
        SegregatedPoolsResourceAllocator::TexturePtr mTexturePtr;
        HAL::TextureProperties mProperties;

        mutable PoolDescriptorAllocator::DSDescriptorPtr mDSDescriptor;
        mutable PoolDescriptorAllocator::SRDescriptorPtr mSRDescriptor;
        mutable std::vector<PoolDescriptorAllocator::RTDescriptorPtr> mRTDescriptors;
        mutable std::vector<PoolDescriptorAllocator::UADescriptorPtr> mUADescriptors;

    public:
        inline const auto& Properties() const { return mProperties; }
    };

}

