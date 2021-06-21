#include "MaterialLoader.hpp"

#include <glm/gtc/type_precision.hpp>

namespace PathFinder
{

    MaterialLoader::MaterialLoader(const std::filesystem::path& executableFolderPath, Memory::GPUResourceProducer* resourceProducer)
        : mResourceLoader{ resourceProducer }, mResourceProducer{ resourceProducer }
    {
        CreateDefaultTextures();
        LoadLTCLookupTables(executableFolderPath);
    }

    void MaterialLoader::LoadMaterial(Material& material)
    {
        auto loadTexture = [this](Material::TextureData& textureData)
        {
            if (!std::filesystem::exists(textureData.FilePath))
                return;

            textureData.Texture = mResourceLoader.LoadTexture(textureData.FilePath, true);
            textureData.RowMajorBlob = std::move(mResourceLoader.RowMajorBlob());
        };

        loadTexture(material.DiffuseAlbedoMap);
        loadTexture(material.SpecularAlbedoMap);
        loadTexture(material.NormalMap);
        loadTexture(material.RoughnessMap);
        loadTexture(material.MetalnessMap);
        loadTexture(material.TranslucencyMap);
        loadTexture(material.DisplacementMap);
        loadTexture(material.DistanceField);

        SetCommonMaterialTextures(material);
    }

    void MaterialLoader::SetCommonMaterialTextures(Material& material)
    {
        if (!material.DiffuseAlbedoMap.Texture)
            material.DiffuseAlbedoMap.Texture = Memory::GPUResourceProducer::TexturePtr{ m1x1White2DTexture.get(), [](Memory::Texture* texture) {} };

        if (!material.SpecularAlbedoMap.Texture)
            material.SpecularAlbedoMap.Texture = Memory::GPUResourceProducer::TexturePtr{ material.DiffuseAlbedoMap.Texture.get(), [](Memory::Texture* texture) {} };

        if (!material.NormalMap.Texture)
            material.NormalMap.Texture = Memory::GPUResourceProducer::TexturePtr{ m1x1White2DTexture.get(), [](Memory::Texture* texture) {} };

        if (!material.RoughnessMap.Texture)
            material.RoughnessMap.Texture = Memory::GPUResourceProducer::TexturePtr{ m1x1White2DTexture.get(), [](Memory::Texture* texture) {} };

        if (!material.MetalnessMap.Texture)
            material.MetalnessMap.Texture = Memory::GPUResourceProducer::TexturePtr{ m1x1Black2DTexture.get(), [](Memory::Texture* texture) {} };

        if (!material.TranslucencyMap.Texture)
            material.TranslucencyMap.Texture = Memory::GPUResourceProducer::TexturePtr{ m1x1Black2DTexture.get(), [](Memory::Texture* texture) {} };

        if (!material.DisplacementMap.Texture)
            material.DisplacementMap.Texture = Memory::GPUResourceProducer::TexturePtr{ m1x1Black2DTexture.get(), [](Memory::Texture* texture) {} };

        if (!material.DistanceField.Texture)
            material.DistanceField.Texture = Memory::GPUResourceProducer::TexturePtr{ m1x1Black3DTexture.get(), [](Memory::Texture* texture) {} };

        material.LTC_LUT_MatrixInverse_Specular = mLTC_LUT_MatrixInverse_GGXHeightCorrelated.get();
        material.LTC_LUT_Matrix_Specular = mLTC_LUT_Matrix_GGXHeightCorrelated.get();
        material.LTC_LUT_Terms_Specular = mLTC_LUT_Terms_GGXHeightCorrelated.get();

        material.LTC_LUT_MatrixInverse_Diffuse = mLTC_LUT_MatrixInverse_DisneyDiffuseNormalized.get();
        material.LTC_LUT_Matrix_Diffuse = mLTC_LUT_Matrix_DisneyDiffuseNormalized.get();
        material.LTC_LUT_Terms_Diffuse = mLTC_LUT_Terms_DisneyDiffuseNormalized.get();
    }

    void MaterialLoader::CreateDefaultTextures()
    {
        HAL::TextureProperties dummy2DTextureProperties{
            HAL::ColorFormat::RGBA8_Unsigned_Norm, HAL::TextureKind::Texture2D,
            Geometry::Dimensions{1, 1}, HAL::ResourceState::AnyShaderAccess };

        HAL::TextureProperties dummy3DTextureProperties{
            HAL::ColorFormat::RGBA8_Unsigned_Norm, HAL::TextureKind::Texture3D,
            Geometry::Dimensions{1, 1}, HAL::ResourceState::AnyShaderAccess };

        m1x1Black2DTexture = mResourceProducer->NewTexture(dummy2DTextureProperties);
        m1x1White2DTexture = mResourceProducer->NewTexture(dummy2DTextureProperties);
        m1x1Black3DTexture = mResourceProducer->NewTexture(dummy3DTextureProperties);

        m1x1Black2DTexture->RequestWrite();
        m1x1White2DTexture->RequestWrite();
        m1x1Black3DTexture->RequestWrite();

        glm::u8vec4 black{ 0, 0, 0, 0 };
        glm::u8vec4 white{ 255, 255, 255, 255 };

        m1x1Black2DTexture->Write(&black, 0, 1);
        m1x1White2DTexture->Write(&white, 0, 1);
        m1x1Black3DTexture->Write(&black, 0, 1);
    }

    void MaterialLoader::LoadLTCLookupTables(const std::filesystem::path& executableFolderPath)
    {
        mLTC_LUT_MatrixInverse_GGXHeightCorrelated = mResourceLoader.LoadTexture(executableFolderPath / "Precompiled" / "LTC_LUT_GGXHeightCorrelated_Matrix_Inverse.dds");
        mLTC_LUT_Matrix_GGXHeightCorrelated = mResourceLoader.LoadTexture(executableFolderPath / "Precompiled" / "LTC_LUT_GGXHeightCorrelated_Matrix.dds");
        mLTC_LUT_Terms_GGXHeightCorrelated = mResourceLoader.LoadTexture(executableFolderPath / "Precompiled" / "LTC_LUT_GGXHeightCorrelated_Terms.dds");

        mLTC_LUT_MatrixInverse_DisneyDiffuseNormalized = mResourceLoader.LoadTexture(executableFolderPath / "Precompiled" / "LTC_LUT_Disney_Diffuse_Normalized_Matrix_Inverse.dds");
        mLTC_LUT_Matrix_DisneyDiffuseNormalized = mResourceLoader.LoadTexture(executableFolderPath / "Precompiled" / "LTC_LUT_Disney_Diffuse_Normalized_Matrix.dds");
        mLTC_LUT_Terms_DisneyDiffuseNormalized = mResourceLoader.LoadTexture(executableFolderPath / "Precompiled" / "LTC_LUT_Disney_Diffuse_Normalized_Terms.dds");
    }

}
