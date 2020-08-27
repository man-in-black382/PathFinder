#pragma once

#include "../Memory/GPUResourceProducer.hpp"
#include "../HardwareAbstractionLayer/Texture.hpp"
#include "../ThirdParty/dds/dds-ktx.h"

#include <filesystem>
#include <vector>

namespace PathFinder 
{

    class ResourceLoader
    {
    public:
        ResourceLoader(const std::filesystem::path& rootPath, Memory::GPUResourceProducer* resourceProducer);

        Memory::GPUResourceProducer::TexturePtr LoadTexture(const std::string& relativeFilePath) const;
        void StoreResource(const Memory::GPUResource& resource, const std::string& relativeFilePath) const;

    private:
        HAL::TextureKind ToKind(const ddsktx_texture_info& textureInfo) const;
        HAL::FormatVariant ToResourceFormat(const ddsktx_format& parserFormat) const;
        Memory::GPUResourceProducer::TexturePtr AllocateTexture(const ddsktx_texture_info& textureInfo) const;

        std::filesystem::path mRootPath;
        Memory::GPUResourceProducer* mResourceProducer;
    };

}
