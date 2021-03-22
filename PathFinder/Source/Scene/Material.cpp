#include "Material.hpp"

#include <bitsery/adapter/stream.h>
#include <bitsery/traits/vector.h>
#include <fstream>
#include <Utility/SerializationAdapters.hpp>

namespace PathFinder
{

    void Material::SerializeTextures(const std::filesystem::path& filePath)
    {
        std::fstream stream{ filePath, std::ios::binary | std::ios::trunc | std::ios::out };

        assert_format(stream.is_open(), "File (", filePath.string(), ") couldn't be opened for writing");

        HAL::TextureProperties dummyProperties{ HAL::ColorFormat::R8_Unsigned_Norm, HAL::TextureKind::Texture2D, Geometry::Dimensions{1}, HAL::ResourceState::Common };
        bitsery::Serializer<bitsery::OutputBufferedStreamAdapter> ser{ stream };

        auto serializeTextureBlob = [&ser, &dummyProperties](Material::TextureData& textureData)
        {
            HAL::TextureProperties textureProperties = (textureData.RowMajorBlob.empty() || !textureData.Texture) ? dummyProperties : textureData.Texture->Properties();
            ser.object(textureProperties);
            ser.container1b(textureData.RowMajorBlob, std::numeric_limits<uint64_t>::max());
            textureData.RowMajorBlob.clear();
        };

        serializeTextureBlob(DiffuseAlbedoMap);
        serializeTextureBlob(SpecularAlbedoMap);
        serializeTextureBlob(NormalMap);
        serializeTextureBlob(RoughnessMap);
        serializeTextureBlob(MetalnessMap);
        serializeTextureBlob(TranslucencyMap);
        serializeTextureBlob(AOMap);
        serializeTextureBlob(DisplacementMap);
        serializeTextureBlob(DistanceField);

        ser.adapter().flush();

        stream.close();
    }

    void Material::DeserializeTextures(const std::filesystem::path& filePath, Memory::GPUResourceProducer* resourceProducer)
    {
        std::fstream stream{ filePath, std::ios::binary | std::ios::in };
        assert_format(stream.is_open(), "File (", filePath.string(), ") couldn't be opened for reading");

        bitsery::Deserializer<bitsery::InputStreamAdapter> des{ stream };

        auto deserializeTextureBlob = [&des, resourceProducer](Material::TextureData& textureData)
        {
            HAL::TextureProperties textureProperties{ HAL::ColorFormat::R8_Unsigned_Norm, HAL::TextureKind::Texture2D, Geometry::Dimensions{1}, HAL::ResourceState::Common };
            des.object(textureProperties);
            des.container1b(textureData.RowMajorBlob, std::numeric_limits<uint64_t>::max());

            if (textureData.RowMajorBlob.empty())
                return;

            textureData.Texture = resourceProducer->NewTexture(textureProperties);

            assert_format(textureData.RowMajorBlob.size() == textureData.Texture->Footprint().TotalSizeInBytes(), "Serialized blob does not match resource size");

            textureData.Texture->RequestWrite();
            textureData.Texture->Write(textureData.RowMajorBlob.data(), 0, textureData.Texture->Footprint().TotalSizeInBytes());
            textureData.RowMajorBlob.clear();
        };

        deserializeTextureBlob(DiffuseAlbedoMap);
        deserializeTextureBlob(SpecularAlbedoMap);
        deserializeTextureBlob(NormalMap);
        deserializeTextureBlob(RoughnessMap);
        deserializeTextureBlob(MetalnessMap);
        deserializeTextureBlob(TranslucencyMap);
        deserializeTextureBlob(AOMap);
        deserializeTextureBlob(DisplacementMap);
        deserializeTextureBlob(DistanceField);
    }

    bool Material::IsTransparent() const
    {
        if (TranslucencyMap.Texture)
            return true;

        //bool hasNonZeroTransmissionFilter = TransmissionFilter && glm::any(glm::greaterThan(*TransmissionFilter, glm::vec3{ 0.05f }));
        bool hasNonZeroTranslucency = TranslucencyOverride && *TranslucencyOverride > 0.05f;

        return hasNonZeroTranslucency;
    }

}
