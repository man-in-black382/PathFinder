#define DDSKTX_IMPLEMENT

#include "TextureLoader.hpp"

#include "../Foundation/Assert.hpp"

#include <fstream>
#include <iterator>
#include <vector>

namespace PathFinder
{

    TextureLoader::TextureLoader(const std::filesystem::path& rootTexturePath, const HAL::Device* device, CopyDevice* copyDevice)
        : mRootPath{ rootTexturePath }, mDevice{ device }, mCopyDevice{ copyDevice } {}

    std::shared_ptr<HAL::TextureResource> TextureLoader::Load(const std::string& relativeFilePath) const
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

        assert_format(textureInfo.num_layers == 1, "Texture array are not supported yet");

        std::shared_ptr<HAL::TextureResource> texture = AllocateTexture(textureInfo);
        HAL::ResourceFootprint textureFootprint{ *texture };

        auto uploadBuffer = std::make_shared<HAL::BufferResource<uint8_t>>(
            *mDevice, textureFootprint.TotalSizeInBytes(), 1, HAL::CPUAccessibleHeapType::Upload);

        uint64_t bufferMemoryOffset = 0;

        for (int mip = 0; mip < textureInfo.num_mips; mip++)
        {
            ddsktx_sub_data subData;
            ddsktx_get_sub(&textureInfo, &subData, bytes.data(), (int)bytes.size(), 0, 0, mip);

            const HAL::SubresourceFootprint& mipFootprint = textureFootprint.GetSubresourceFootprint(mip);

            bool imageDataSatisfiesTextureRowAlignment = mipFootprint.RowPitch() == subData.row_pitch_bytes;

            if (imageDataSatisfiesTextureRowAlignment)
            {
                // Copy whole subresource
                uploadBuffer->Write(bufferMemoryOffset, (uint8_t*)subData.buff, subData.size_bytes);
                bufferMemoryOffset += subData.size_bytes;
            }
            else {
                // Have to copy row-by-row
                uint8_t* rowData = (uint8_t*)subData.buff;

                for (auto row = 0; row < subData.height; ++row)
                {
                    uploadBuffer->Write(bufferMemoryOffset, (uint8_t*)subData.buff, subData.row_pitch_bytes);
                    bufferMemoryOffset += mipFootprint.RowPitch();
                }
            }  
        }

        mCopyDevice->QueueBufferToTextureCopy(uploadBuffer, texture, textureFootprint);
        texture->SetDebugName(fullPath.filename().string());

        input.close();
        
        return std::move(texture);
    }

    HAL::ResourceFormat::TextureKind TextureLoader::ToKind(const ddsktx_texture_info& textureInfo) const
    {
        bool isArray = textureInfo.num_layers > 1;

        if (textureInfo.depth > 1 && !isArray) return HAL::ResourceFormat::TextureKind::Texture3D;
        if (textureInfo.depth > 1 || textureInfo.width > 1) return HAL::ResourceFormat::TextureKind::Texture2D;

        return HAL::ResourceFormat::TextureKind::Texture1D;
    }

    HAL::ResourceFormat::FormatVariant TextureLoader::ToResourceFormat(const ddsktx_format& parserFormat) const
    {
        switch (parserFormat)
        {
        case DDSKTX_FORMAT_R8:          return HAL::ResourceFormat::Color::R8_Usigned_Norm;
        case DDSKTX_FORMAT_RGBA8:       return HAL::ResourceFormat::Color::RGBA8_Usigned_Norm;
        case DDSKTX_FORMAT_RGBA8S:      return HAL::ResourceFormat::Color::RGBA8_Signed;
        case DDSKTX_FORMAT_RG16:        return HAL::ResourceFormat::Color::RG16_Unsigned;
        case DDSKTX_FORMAT_RGB8:        return HAL::ResourceFormat::Color::RGBA8_Unsigned;
        case DDSKTX_FORMAT_R16:         return HAL::ResourceFormat::Color::R16_Unsigned;
        case DDSKTX_FORMAT_R32F:        return HAL::ResourceFormat::Color::R32_Float;
        case DDSKTX_FORMAT_R16F:        return HAL::ResourceFormat::Color::R16_Float;
        case DDSKTX_FORMAT_RG16F:       return HAL::ResourceFormat::Color::RG16_Float;
        case DDSKTX_FORMAT_RG16S:       return HAL::ResourceFormat::Color::RG16_Signed;
        case DDSKTX_FORMAT_RGBA16F:     return HAL::ResourceFormat::Color::RGBA16_Float;
        case DDSKTX_FORMAT_RGBA16:      return HAL::ResourceFormat::Color::RGBA16_Unsigned;
        case DDSKTX_FORMAT_RG8:         return HAL::ResourceFormat::Color::RG8_Usigned_Norm;
        case DDSKTX_FORMAT_RG8S:        return HAL::ResourceFormat::Color::RG8_Signed;
        case DDSKTX_FORMAT_BGRA8:       return HAL::ResourceFormat::Color::BGRA8_Unsigned_Norm;

        case DDSKTX_FORMAT_RGB10A2:
        case DDSKTX_FORMAT_RG11B10F:
        case DDSKTX_FORMAT_A8:
        case DDSKTX_FORMAT_BC1:         // DXT1
        case DDSKTX_FORMAT_BC2:         // DXT3
        case DDSKTX_FORMAT_BC3:         // DXT5
        case DDSKTX_FORMAT_BC4:         // ATI1
        case DDSKTX_FORMAT_BC5:         // ATI2
        case DDSKTX_FORMAT_BC6H:        // BC6H
        case DDSKTX_FORMAT_BC7:         // BC7
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
            return HAL::ResourceFormat::Color::R16_Float;
        }
    }

    std::shared_ptr<HAL::TextureResource> TextureLoader::AllocateTexture(const ddsktx_texture_info& textureInfo) const
    {
        HAL::ResourceFormat::FormatVariant format = ToResourceFormat(textureInfo.format);
        Geometry::Dimensions dimensions(textureInfo.width, textureInfo.height, textureInfo.depth);
        HAL::ResourceFormat::TextureKind kind = ToKind(textureInfo);
        HAL::ResourceFormat::ColorClearValue clearValue{ 0.0, 0.0, 0.0, 0.0 };

        return std::make_shared<HAL::TextureResource>(
            *mDevice, format, kind, dimensions, clearValue,
            HAL::ResourceState::CopyDestination,
            HAL::ResourceState::PixelShaderAccess | 
            HAL::ResourceState::NonPixelShaderAccess,
            textureInfo.num_mips);
    }

}
