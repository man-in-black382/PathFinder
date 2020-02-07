#include "Texture.hpp"

namespace Memory
{

    Texture::Texture(
        const HAL::Texture::Properties& properties, 
        ResourceStateTracker* stateTracker,
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator,
        HAL::CopyCommandListBase* commandList)
        :
        GPUResource(UploadStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, commandList),
        mTexturePtr{ resourceAllocator->AllocateTexture(properties) } 
    {
        if (StateTracker()) StateTracker()->StartTrakingResource(HALTexture());
    }

    Texture::Texture(
        const HAL::Texture::Properties& properties, 
        ResourceStateTracker* stateTracker, 
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator, 
        HAL::CopyCommandListBase* commandList,
        const HAL::Device& device, 
        const HAL::Heap& mainResourceExplicitHeap, 
        uint64_t explicitHeapOffset)
        :
        GPUResource(UploadStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, commandList)
    {
        mTexturePtr = SegregatedPoolsResourceAllocator::TexturePtr{
           new HAL::Texture{ device, mainResourceExplicitHeap, explicitHeapOffset, properties },
           [](HAL::Texture* texture) { delete texture; }
        };
    }

    Texture::~Texture()
    {
        if (StateTracker()) StateTracker()->StopTrakingResource(HALTexture());
    }

    const HAL::RTDescriptor* Texture::GetOrCreateRTDescriptor()
    {
        if (!mRTDescriptor) mRTDescriptor = DescriptorAllocator()->AllocateRTDescriptor(HALTexture());
        return mRTDescriptor.get();
    }

    const HAL::DSDescriptor* Texture::GetOrCreateDSDescriptor()
    {
        if (!mDSDescriptor) mDSDescriptor = DescriptorAllocator()->AllocateDSDescriptor(HALTexture());
        return mDSDescriptor.get();
    }

    const HAL::SRDescriptor* Texture::GetOrCreateSRDescriptor()
    {
        if (!mSRDescriptor) mSRDescriptor = DescriptorAllocator()->AllocateSRDescriptor(HALTexture());
        return mSRDescriptor.get();
    }

    const HAL::UADescriptor* Texture::GetOrCreateUADescriptor()
    {
        if (!mUADescriptor) mUADescriptor = DescriptorAllocator()->AllocateUADescriptor(HALTexture());
        return mUADescriptor.get();
    }

    void Texture::RequestWrite()
    {
        GPUResource::RequestWrite();

        HAL::ResourceFootprint footprint{ *HALTexture() };

        for (const HAL::SubresourceFootprint& subresourceFootprint : footprint.SubresourceFootprints())
        {
            CommandList()->CopyBufferToTexture(*CurrentFrameUploadBuffer(), *HALTexture(), subresourceFootprint);
        }
    }

    void Texture::RequestRead()
    {
        GPUResource::RequestRead();

        HAL::ResourceFootprint textureFootprint{ *HALTexture() };

        for (const HAL::SubresourceFootprint& subresourceFootprint : textureFootprint.SubresourceFootprints())
        {
            CommandList()->CopyTextureToBuffer(*HALTexture(), *CurrentFrameReadbackBuffer(), subresourceFootprint);
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

}
