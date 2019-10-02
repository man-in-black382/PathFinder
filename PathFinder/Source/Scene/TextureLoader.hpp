#pragma once

#include "../RenderPipeline/CopyDevice.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../ThirdParty/dds/dds-ktx.h"

#include <filesystem>
#include <memory>

namespace PathFinder 
{

    class TextureLoader
    {
    public:
        TextureLoader(const std::filesystem::path& rootTexturePath, const HAL::Device* device, CopyDevice* copyDevice);

        std::unique_ptr<HAL::TextureResource> Load(const std::string& relativeFilePath) const;

    private:
        HAL::ResourceFormat::TextureKind ToKind(const ddsktx_texture_info& textureInfo) const;
        HAL::ResourceFormat::FormatVariant ToResourceFormat(const ddsktx_format& parserFormat) const;
        std::unique_ptr<HAL::TextureResource> AllocateTexture(const ddsktx_texture_info& textureInfo) const;

        const std::filesystem::path& mRootPath;
        const HAL::Device* mDevice;
        CopyDevice* mCopyDevice;
    };

}
