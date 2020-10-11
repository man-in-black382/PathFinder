#define DDSKTX_IMPLEMENT

#include "ResourceLoader.hpp"



#include <fstream>
#include <iterator>
#include <vector>

namespace PathFinder
{

    ResourceLoader::ResourceLoader(const std::filesystem::path& rootPath, Memory::GPUResourceProducer* resourceProducer)
        : mRootPath{ rootPath }, mResourceProducer{ resourceProducer } {}

    Memory::GPUResourceProducer::TexturePtr ResourceLoader::LoadTexture(const std::string& relativeFilePath) const
    {
        std::filesystem::path fullPath = mRootPath;
        fullPath += relativeFilePath;

        std::ifstream input{ fullPath.string(), std::ios::binary };

        if (!input.is_open())
        {
            return nullptr;
        }
        
        std::uintmax_t fileSize = std::filesystem::file_size(fullPath);

        std::vector<uint8_t> bytes;
        bytes.resize(fileSize);

        input.read((char *)bytes.data(), bytes.size());

        ddsktx_texture_info textureInfo;
        ddsktx_error error;

        if (!ddsktx_parse(&textureInfo, bytes.data(), (int)bytes.size(), &error))
        {
            return nullptr;
        }

        assert_format(textureInfo.num_layers == 1, "Texture arrays are not supported yet");

        auto texture = AllocateTexture(textureInfo);
        HAL::ResourceFootprint textureFootprint{ *texture->HALTexture() };

        texture->RequestWrite();

        uint64_t uploadMemoryOffset = 0;

        for (int mip = 0; mip < textureInfo.num_mips; ++mip)
        {
            const HAL::SubresourceFootprint& mipFootprint = textureFootprint.GetSubresourceFootprint(mip);
            uploadMemoryOffset = mipFootprint.Offset();

            for (int depthLayer = 0; depthLayer < textureInfo.depth; ++depthLayer)
            {
                ddsktx_sub_data subData;
                ddsktx_get_sub(&textureInfo, &subData, bytes.data(), (int)bytes.size(), 0, depthLayer, mip);

                bool imageDataSatisfiesTextureRowAlignment =
                    mipFootprint.RowPitch() == subData.row_pitch_bytes &&
                    mipFootprint.RowSizeInBytes() == subData.row_pitch_bytes;

                if (imageDataSatisfiesTextureRowAlignment)
                {
                    // Copy whole subresource
                    texture->Write((uint8_t*)subData.buff, uploadMemoryOffset, subData.size_bytes);
                    uploadMemoryOffset += subData.size_bytes;
                }
                else {
                    // Have to copy row-by-row
                    uint8_t* rowData = (uint8_t*)subData.buff;
                    uint64_t rowReadCountAtOnce = mipFootprint.RowSizeInBytes() / subData.row_pitch_bytes;
                    uint64_t readMemoryOffset = 0;

                    for (auto row = 0; row < subData.height; row += rowReadCountAtOnce)
                    {
                        uint64_t bytesToRead = subData.row_pitch_bytes * rowReadCountAtOnce;
                        texture->Write((uint8_t*)subData.buff + readMemoryOffset, uploadMemoryOffset, bytesToRead);
                        uploadMemoryOffset += mipFootprint.RowPitch();
                        readMemoryOffset += bytesToRead;
                    }
                }
            }
        }

        texture->SetDebugName(fullPath.filename().string());
        input.close();
        
        return std::move(texture);
    }

    void ResourceLoader::StoreResource(const Memory::GPUResource& resource, const std::string& relativeFilePath) const
    {

    }

    HAL::TextureKind ResourceLoader::ToKind(const ddsktx_texture_info& textureInfo) const
    {
        bool isArray = textureInfo.num_layers > 1;

        if (textureInfo.depth > 1 && !isArray) return HAL::TextureKind::Texture3D;
        if (textureInfo.depth > 1 || textureInfo.width > 1) return HAL::TextureKind::Texture2D;

        return HAL::TextureKind::Texture1D;
    }

    HAL::FormatVariant ResourceLoader::ToResourceFormat(const ddsktx_format& parserFormat) const
    {
        switch (parserFormat)
        {
            // Supported color formats
        case DDSKTX_FORMAT_R8:          return HAL::ColorFormat::R8_Unsigned_Norm;
        case DDSKTX_FORMAT_RGBA8:       return HAL::ColorFormat::RGBA8_Usigned_Norm;
        case DDSKTX_FORMAT_RGBA8S:      return HAL::ColorFormat::RGBA8_Signed;
        case DDSKTX_FORMAT_RG16:        return HAL::ColorFormat::RG16_Unsigned;
        case DDSKTX_FORMAT_RGB8:        return HAL::ColorFormat::RGBA8_Unsigned;
        case DDSKTX_FORMAT_R16:         return HAL::ColorFormat::R16_Unsigned;
        case DDSKTX_FORMAT_R32F:        return HAL::ColorFormat::R32_Float;
        case DDSKTX_FORMAT_R16F:        return HAL::ColorFormat::R16_Float;
        case DDSKTX_FORMAT_RG16F:       return HAL::ColorFormat::RG16_Float;
        case DDSKTX_FORMAT_RG16S:       return HAL::ColorFormat::RG16_Signed;
        case DDSKTX_FORMAT_RGBA16F:     return HAL::ColorFormat::RGBA16_Float;
        case DDSKTX_FORMAT_RGBA16:      return HAL::ColorFormat::RGBA16_Unsigned_Norm;
        case DDSKTX_FORMAT_RG8:         return HAL::ColorFormat::RG8_Usigned_Norm;
        case DDSKTX_FORMAT_RG8S:        return HAL::ColorFormat::RG8_Signed;
        case DDSKTX_FORMAT_BGRA8:       return HAL::ColorFormat::BGRA8_Unsigned_Norm;

            // Supported compressed formats
        case DDSKTX_FORMAT_BC1:         return HAL::ColorFormat::BC1_Unsigned_Norm;
        case DDSKTX_FORMAT_BC2:         return HAL::ColorFormat::BC2_Unsigned_Norm;
        case DDSKTX_FORMAT_BC3:         return HAL::ColorFormat::BC3_Unsigned_Norm;
        case DDSKTX_FORMAT_BC4:         return HAL::ColorFormat::BC4_Unsigned_Norm;
        case DDSKTX_FORMAT_BC5:         return HAL::ColorFormat::BC5_Unsigned_Norm;
        case DDSKTX_FORMAT_BC7:         return HAL::ColorFormat::BC7_Unsigned_Norm;

            // Not yet supported formats
        case DDSKTX_FORMAT_RGB10A2:
        case DDSKTX_FORMAT_RG11B10F:
        case DDSKTX_FORMAT_A8:
        case DDSKTX_FORMAT_BC6H:        // BC6H
        case DDSKTX_FORMAT_ETC1:        // ETC1 RGB8
        case DDSKTX_FORMAT_ETC2:        // ETC2 RGB8
        case DDSKTX_FORMAT_ETC2A:       // ETC2 RGBA8
        case DDSKTX_FORMAT_ETC2A1:      // ETC2 RGBA8A1
        case DDSKTX_FORMAT_PTC12:       // PVRTC1 RGB 2bpp
        case DDSKTX_FORMAT_PTC14:       // PVRTC1 RGB 4bpp
        case DDSKTX_FORMAT_PTC12A:      // PVRTC1 RGBA 2bpp
        case DDSKTX_FORMAT_PTC14A:      // PVRTC1 RGBA 4bpp
        case DDSKTX_FORMAT_PTC22:       // PVRTC2 RGBA 2bpp
        case DDSKTX_FORMAT_PTC24:       // PVRTC2 RGBA 4bpp
        case DDSKTX_FORMAT_ATC:         // ATC RGB 4BPP
        case DDSKTX_FORMAT_ATCE:        // ATCE RGBA 8 BPP explicit alpha
        case DDSKTX_FORMAT_ATCI:        // ATCI RGBA 8 BPP interpolated alpha
        case DDSKTX_FORMAT_ASTC4x4:     // ASTC 4x4 8.0 BPP
        case DDSKTX_FORMAT_ASTC5x5:     // ASTC 5x5 5.12 BPP
        case DDSKTX_FORMAT_ASTC6x6:     // ASTC 6x6 3.56 BPP
        case DDSKTX_FORMAT_ASTC8x5:     // ASTC 8x5 3.20 BPP
        case DDSKTX_FORMAT_ASTC8x6:     // ASTC 8x6 2.67 BPP
        case DDSKTX_FORMAT_ASTC10x5:    // ASTC 10x5 2.56 BPP
        default: 
            assert_format(false, "Format is not supported at the moment"); 
            return HAL::ColorFormat::R16_Float;
        }
    }

    Memory::GPUResourceProducer::TexturePtr ResourceLoader::AllocateTexture(const ddsktx_texture_info& textureInfo) const
    {
        HAL::FormatVariant format = ToResourceFormat(textureInfo.format);
        Geometry::Dimensions dimensions(textureInfo.width, textureInfo.height, textureInfo.depth);
        HAL::TextureKind kind = ToKind(textureInfo);

        HAL::TextureProperties properties{ format, kind, dimensions, HAL::ResourceState::AnyShaderAccess, (uint16_t)textureInfo.num_mips };

        return mResourceProducer->NewTexture(properties);
    }

}
