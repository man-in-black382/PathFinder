#pragma once

#include "GPUResource.hpp"

#include "../HardwareAbstractionLayer/Texture.hpp"

namespace Memory
{
   
    class Texture : public GPUResource
    {
    public:
        Texture(const HAL::Texture::Properties& properties, SegregatedPoolsResourceAllocator* resourceAllocator, HAL::CopyCommandListBase* commandList);
        ~Texture() = default;

        void RequestWrite() override;
        void RequestRead() override;

        const HAL::Texture* HALTexture() const;
        const HAL::Resource* HALResource() const override;

    private:
        SegregatedPoolsResourceAllocator::TexturePtr mTexturePtr;
    };

}

