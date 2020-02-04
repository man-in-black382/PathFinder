#include "Texture.hpp"

namespace Memory
{

    Texture::Texture(const HAL::Texture::Properties& properties, SegregatedPoolsResourceAllocator* resourceAllocator, HAL::CopyCommandListBase* commandList)
        : GPUResource(GPUResource::UploadStrategy::Automatic, resourceAllocator, commandList),
        mTexturePtr{ resourceAllocator->AllocateTexture(properties) } {}

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
