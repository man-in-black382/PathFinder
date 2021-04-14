#pragma once

#include <Memory/GPUResourceProducer.hpp>
#include <Utility/SerializationAdapters.hpp>
#include <bitsery/ext/std_optional.h>

namespace PathFinder 
{

    class Material
    {
    public:
        enum class WrapMode : uint8_t
        {
            Clamp, Mirror, Repeat
        };

        struct TextureData
        {
            std::filesystem::path FilePath;
            Memory::GPUResourceProducer::TexturePtr Texture;
            WrapMode Wrapping = WrapMode::Repeat;
            std::vector<uint8_t> RowMajorBlob;

            template <typename S>
            void serialize(S& s)
            {
                s.value1b(Wrapping);
            }
        };

        void SerializeTextures(const std::filesystem::path& filePath);
        void DeserializeTextures(const std::filesystem::path& filePath, Memory::GPUResourceProducer* resourceProducer);

        bool IsTransparent() const;

        TextureData DiffuseAlbedoMap;
        TextureData SpecularAlbedoMap;
        TextureData NormalMap;
        TextureData RoughnessMap;
        TextureData MetalnessMap;
        TextureData TranslucencyMap;
        TextureData DisplacementMap;
        TextureData DistanceField;

        Memory::Texture* LTC_LUT_MatrixInverse_Specular = nullptr;
        Memory::Texture* LTC_LUT_Matrix_Specular = nullptr;
        Memory::Texture* LTC_LUT_Terms_Specular = nullptr;
        Memory::Texture* LTC_LUT_MatrixInverse_Diffuse = nullptr;
        Memory::Texture* LTC_LUT_Matrix_Diffuse = nullptr;
        Memory::Texture* LTC_LUT_Terms_Diffuse = nullptr;

        std::optional<glm::vec3> DiffuseAlbedoOverride;
        std::optional<glm::vec3> SpecularAlbedoOverride;
        std::optional<float> RoughnessOverride;
        std::optional<float> MetalnessOverride;
        std::optional<float> TranslucencyOverride;
        std::optional<float> IOROverride; // Index of refraction
        std::optional<glm::vec3> TransmissionFilter; // Allows the specific colors to pass through

        std::string Name;
        uint32_t GPUMaterialTableIndex = 0;

        template <typename S>
        void serialize(S& s)
        {
            s.container1b(Name, 1000);
            s.object(DiffuseAlbedoMap);
            s.object(SpecularAlbedoMap);
            s.object(NormalMap);
            s.object(RoughnessMap);
            s.object(MetalnessMap);
            s.object(DisplacementMap);
            s.object(DistanceField);
            s.ext(DiffuseAlbedoOverride, bitsery::ext::StdOptional{});
            s.ext(SpecularAlbedoOverride, bitsery::ext::StdOptional{});
            s.ext4b(RoughnessOverride, bitsery::ext::StdOptional{});
            s.ext4b(MetalnessOverride, bitsery::ext::StdOptional{});
            s.ext4b(TranslucencyOverride, bitsery::ext::StdOptional{});
            s.ext4b(IOROverride, bitsery::ext::StdOptional{});
            s.ext(TransmissionFilter, bitsery::ext::StdOptional{});
        }
    };

}
