#pragma once

#include <Memory/GPUResourceProducer.hpp>
#include <HardwareAbstractionLayer/Texture.hpp>
#include <ThirdParty/dds/dds-ktx.h>

#include <filesystem>
#include <vector>

namespace PathFinder 
{

    class ResourceLoader
    {
    public:
        ResourceLoader(Memory::GPUResourceProducer* resourceProducer);

        Memory::GPUResourceProducer::TexturePtr LoadTexture(const std::filesystem::path& path, bool saveRowMajorBlob = false);
        void StoreResource(const Memory::GPUResource& resource, const std::filesystem::path& path) const;

    private:
        HAL::TextureKind ToKind(const ddsktx_texture_info& textureInfo) const;
        HAL::FormatVariant ToResourceFormat(const ddsktx_format& parserFormat) const;
        Memory::GPUResourceProducer::TexturePtr AllocateTexture(const ddsktx_texture_info& textureInfo) const;

        std::vector<uint8_t> mRowMajorBlob;
        Memory::GPUResourceProducer* mResourceProducer;

    public:
        inline auto& RowMajorBlob() { return mRowMajorBlob; }
    };

}
