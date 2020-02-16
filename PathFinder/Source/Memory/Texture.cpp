#include "Texture.hpp"

namespace Memory
{

    Texture::Texture(
        const HAL::Texture::Properties& properties, 
        ResourceStateTracker* stateTracker,
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator,
        CopyCommandListProvider* commandListProvider)
        :
        GPUResource(UploadStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, commandListProvider),
        mTexturePtr{ resourceAllocator->AllocateTexture(properties) } 
    {
        if (mStateTracker) mStateTracker->StartTrakingResource(HALTexture());
    }

    Texture::Texture(
        const HAL::Texture::Properties& properties, 
        ResourceStateTracker* stateTracker, 
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator, 
        CopyCommandListProvider* commandListProvider,
        const HAL::Device& device, 
        const HAL::Heap& mainResourceExplicitHeap, 
        uint64_t explicitHeapOffset)
        :
        GPUResource(UploadStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, commandListProvider)
    {
        mTexturePtr = SegregatedPoolsResourceAllocator::TexturePtr{
           new HAL::Texture{ device, mainResourceExplicitHeap, explicitHeapOffset, properties },
           [](HAL::Texture* texture) { delete texture; }
        };

        if (mStateTracker) mStateTracker->StartTrakingResource(HALTexture());
    }

    Texture::~Texture()
    {
        if (mStateTracker) mStateTracker->StopTrakingResource(HALTexture());
    }

    const HAL::RTDescriptor* Texture::GetOrCreateRTDescriptor()
    {
        if (!mRTDescriptor) mRTDescriptor = mDescriptorAllocator->AllocateRTDescriptor(*HALTexture());
        return mRTDescriptor.get();
    }

    const HAL::DSDescriptor* Texture::GetOrCreateDSDescriptor()
    {
        if (!mDSDescriptor) mDSDescriptor = mDescriptorAllocator->AllocateDSDescriptor(*HALTexture());
        return mDSDescriptor.get();
    }

    const HAL::SRDescriptor* Texture::GetOrCreateSRDescriptor()
    {
        if (!mSRDescriptor) mSRDescriptor = mDescriptorAllocator->AllocateSRDescriptor(*HALTexture());
        return mSRDescriptor.get();
    }

    const HAL::UADescriptor* Texture::GetOrCreateUADescriptor()
    {
        if (!mUADescriptor) mUADescriptor = mDescriptorAllocator->AllocateUADescriptor(*HALTexture());
        return mUADescriptor.get();
    }

    void Texture::RequestWrite()
    {
        GPUResource::RequestWrite();

        HAL::ResourceFootprint footprint{ *HALTexture() };

        for (const HAL::SubresourceFootprint& subresourceFootprint : footprint.SubresourceFootprints())
        {
            mCommandListProvider->CommandList()->CopyBufferToTexture(*CurrentFrameUploadBuffer(), *HALTexture(), subresourceFootprint);
        }
    }

    void Texture::RequestRead()
    {
        GPUResource::RequestRead();

        HAL::ResourceFootprint textureFootprint{ *HALTexture() };

        for (const HAL::SubresourceFootprint& subresourceFootprint : textureFootprint.SubresourceFootprints())
        {
            mCommandListProvider->CommandList()->CopyTextureToBuffer(*HALTexture(), *CurrentFrameReadbackBuffer(), subresourceFootprint);
        }
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

}
