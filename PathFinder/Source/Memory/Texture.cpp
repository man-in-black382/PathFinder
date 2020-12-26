#include "Texture.hpp"
#include "CopyRequestManager.hpp"

namespace Memory
{

    Texture::Texture(
        const HAL::TextureProperties& properties, 
        ResourceStateTracker* stateTracker,
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator,
        CopyRequestManager* copyRequestManager)
        :
        GPUResource(AccessStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, copyRequestManager),
        mTexturePtr{ resourceAllocator->AllocateTexture(properties) },
        mProperties{ properties }
    {
        if (mStateTracker) mStateTracker->StartTrakingResource(mTexturePtr.get());
        ReserveDiscriptorArrays(properties.MipCount);
    }

    Texture::Texture(
        const HAL::TextureProperties& properties, 
        ResourceStateTracker* stateTracker, 
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator, 
        CopyRequestManager* copyRequestManager,
        const HAL::Device& device, 
        const HAL::Heap& mainResourceExplicitHeap, 
        uint64_t explicitHeapOffset)
        :
        GPUResource(AccessStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, copyRequestManager),
        mProperties{ properties }
    {
        mTexturePtr = SegregatedPoolsResourceAllocator::TexturePtr{
           new HAL::Texture{ device, mainResourceExplicitHeap, explicitHeapOffset, properties },
           [](HAL::Texture* texture) { delete texture; }
        };

        if (mStateTracker) mStateTracker->StartTrakingResource(mTexturePtr.get());
        ReserveDiscriptorArrays(properties.MipCount);
    }

    Texture::Texture(
        ResourceStateTracker* stateTracker, 
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator, 
        CopyRequestManager* copyRequestManager,
        HAL::Texture* existingTexture)
        :
        GPUResource(AccessStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, copyRequestManager),
        mProperties{
            existingTexture->Format(), existingTexture->Kind(),
            existingTexture->Dimensions(), existingTexture->OptimizedClearValue(),
            existingTexture->InitialStates(), existingTexture->ExpectedStates()
        }
    {
        mTexturePtr = SegregatedPoolsResourceAllocator::TexturePtr{ existingTexture, [](HAL::Texture* texture) {} };
        if (mStateTracker) mStateTracker->StartTrakingResource(mTexturePtr.get());
        ReserveDiscriptorArrays(1);
    }

    Texture::~Texture()
    {
        if (mStateTracker) mStateTracker->StopTrakingResource(mTexturePtr.get());
    }

    const HAL::RTDescriptor* Texture::GetRTDescriptor(uint8_t mipLevel) const
    {   
        assert_format(mipLevel < mRTDescriptors.size(), "Requested RT descriptor mip exceeds texture's amount of mip levels");

        if (!mRTDescriptors[mipLevel])
        {
            mRTDescriptors[mipLevel] = mDescriptorAllocator->AllocateRTDescriptor(*HALTexture(), mipLevel);
        }
            
        return mRTDescriptors[mipLevel].get();
    }

    const HAL::DSDescriptor* Texture::GetDSDescriptor() const
    {
        if (!mDSDescriptor) mDSDescriptor = mDescriptorAllocator->AllocateDSDescriptor(*HALTexture());
        return mDSDescriptor.get();
    }

    const HAL::SRDescriptor* Texture::GetSRDescriptor() const
    {
        if (!mSRDescriptor)
        {
            mSRDescriptor = mDescriptorAllocator->AllocateSRDescriptor(*HALTexture());
        }

        return mSRDescriptor.get();
    }

    const HAL::UADescriptor* Texture::GetUADescriptor(uint8_t mipLevel) const
    {
        assert_format(mipLevel < mUADescriptors.size(), "Requested UA descriptor mip exceeds texture's amount of mip levels");

        if (!mUADescriptors[mipLevel])
        {
            mUADescriptors[mipLevel] = mDescriptorAllocator->AllocateUADescriptor(*HALTexture(), mipLevel);
        }

        return mUADescriptors[mipLevel].get();
    }

    const HAL::Texture* Texture::HALTexture() const
    {
        return mTexturePtr.get();
    }

    const HAL::Resource* Texture::HALResource() const
    {
        return mTexturePtr.get();
    }

    uint64_t Texture::ResourceSizeInBytes() const
    {
        return mTexturePtr->TotalMemory();
    }

    void Texture::ApplyDebugName()
    {
        GPUResource::ApplyDebugName();

        if (mTexturePtr)
        {
            mTexturePtr->SetDebugName(mDebugName);
        }
    }

    CopyRequestManager::CopyCommand Texture::GetUploadCommands()
    {
        return [&](HAL::CopyCommandListBase& cmdList)
        {
            HAL::ResourceFootprint footprint{ *HALTexture() };

            for (const HAL::SubresourceFootprint& subresourceFootprint : footprint.SubresourceFootprints())
            {
                cmdList.CopyBufferToTexture(*CurrentFrameUploadBuffer(), *HALTexture(), subresourceFootprint);
            }
        };
    }

    CopyRequestManager::CopyCommand Texture::GetReadbackCommands()
    {
        return [&](HAL::CopyCommandListBase& cmdList)
        {
            HAL::ResourceFootprint textureFootprint{ *HALTexture() };

            for (const HAL::SubresourceFootprint& subresourceFootprint : textureFootprint.SubresourceFootprints())
            {
                cmdList.CopyTextureToBuffer(*HALTexture(), *CurrentFrameReadbackBuffer(), subresourceFootprint);
            }
        };
    }

    void Texture::ReserveDiscriptorArrays(uint8_t mipCount)
    {
        mRTDescriptors.resize(mipCount);
        mUADescriptors.resize(mipCount);
    }

}
