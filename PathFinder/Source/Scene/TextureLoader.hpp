#pragma once

#include "../HardwareAbstractionLayer/Texture.hpp"
#include "../ThirdParty/dds/dds-ktx.h"

#include <filesystem>
#include <memory>

namespace PathFinder 
{

    class TextureLoader
    {
    public:
        TextureLoader(const std::filesystem::path& rootTexturePath, const HAL::Device* device);

        std::shared_ptr<HAL::Texture> Load(const std::string& relativeFilePath) const;

    private:
        HAL::TextureKind ToKind(const ddsktx_texture_info& textureInfo) const;
        HAL::ResourceFormat::FormatVariant ToResourceFormat(const ddsktx_format& parserFormat) const;
        std::shared_ptr<HAL::Texture> AllocateTexture(const ddsktx_texture_info& textureInfo) const;

        std::filesystem::path mRootPath;
        const HAL::Device* mDevice;
    };

}
