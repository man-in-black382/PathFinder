#include "MaterialLoader.hpp"

#include <glm/gtc/type_precision.hpp>

namespace PathFinder
{

    MaterialLoader::MaterialLoader(const std::filesystem::path& fileRoot, PreprocessableAssetStorage* assetStorage, Memory::GPUResourceProducer* resourceProducer)
        : mAssetStorage{ assetStorage }, mResourceLoader{ fileRoot, resourceProducer }, mResourceProducer{ resourceProducer }
    {
        HAL::Texture::Properties dummy2DTextureProperties{ 
            HAL::ColorFormat::RGBA8_Usigned_Norm, HAL::TextureKind::Texture2D, 
            Geometry::Dimensions{1, 1}, HAL::ResourceState::AnyShaderAccess };

        HAL::Texture::Properties dummy3DTextureProperties{
            HAL::ColorFormat::RGBA8_Usigned_Norm, HAL::TextureKind::Texture3D,
            Geometry::Dimensions{1, 1}, HAL::ResourceState::AnyShaderAccess };

        m1x1Black2DTexture = mResourceProducer->NewTexture(dummy2DTextureProperties);
        m1x1White2DTexture = mResourceProducer->NewTexture(dummy2DTextureProperties);
        m1x1Black3DTexture = mResourceProducer->NewTexture(dummy3DTextureProperties);

        m1x1Black2DTexture->RequestWrite();
        m1x1White2DTexture->RequestWrite();
        m1x1Black3DTexture->RequestWrite();

        glm::u8vec4 black{ 0, 0, 0, 0 };
        glm::u8vec4 white{ 1, 1, 1, 1 };

        m1x1Black2DTexture->Write(&black, 0, 1);
        m1x1White2DTexture->Write(&white, 0, 1);
        m1x1Black3DTexture->Write(&black, 0, 1);
    }

    Material MaterialLoader::LoadMaterial(
        const std::string& albedoMapRelativePath,
        const std::string& normalMapRelativePath,
        const std::string& roughnessMapRelativePath,
        const std::string& metalnessMapRelativePath,
        std::optional<std::string> displacementMapRelativePath,
        std::optional<std::string> distanceFieldRelativePath,
        std::optional<std::string> AOMapRelativePath)
    {
        Material material{};

        material.AlbedoMap = GetOrAllocateTexture(albedoMapRelativePath);
        material.NormalMap = GetOrAllocateTexture(normalMapRelativePath);
        material.RoughnessMap = GetOrAllocateTexture(roughnessMapRelativePath);
        material.MetalnessMap = GetOrAllocateTexture(metalnessMapRelativePath);

        if (displacementMapRelativePath)
        {
            material.DisplacementMap = GetOrAllocateTexture(*displacementMapRelativePath);
        }

        if (AOMapRelativePath)
        {
            material.AOMap = GetOrAllocateTexture(*AOMapRelativePath);
        }

        if (material.DisplacementMap && distanceFieldRelativePath)
        {
            material.DistanceField = GetOrAllocateTexture(*distanceFieldRelativePath);

            if (!material.DistanceField)
            {
                HAL::Texture::Properties distFieldProperties{
                    HAL::ColorFormat::RGBA32_Unsigned, HAL::TextureKind::Texture3D,
                    DistanceFieldTextureSize, HAL::ResourceState::UnorderedAccess, HAL::ResourceState::AnyShaderAccess };

                material.DistanceField = AllocateAndStoreTexture(distFieldProperties, *distanceFieldRelativePath);

                mAssetStorage->PreprocessAsset(material.DistanceField, [this, distanceFieldRelativePath](Memory::GPUResource* distanceField)
                {
                    mResourceLoader.StoreResource(*distanceField, *distanceFieldRelativePath);
                });
            }
        }

        if (!material.AlbedoMap) material.AlbedoMap = m1x1White2DTexture.get();
        if (!material.NormalMap) material.NormalMap = m1x1White2DTexture.get();
        if (!material.RoughnessMap) material.RoughnessMap = m1x1White2DTexture.get();
        if (!material.MetalnessMap) material.MetalnessMap = m1x1Black2DTexture.get();
        if (!material.DisplacementMap) material.DisplacementMap = m1x1Black2DTexture.get();
        if (!material.AOMap) material.AOMap = m1x1White2DTexture.get();
        if (!material.DistanceField) material.DistanceField = m1x1Black3DTexture.get();

        return material;
    }

    Memory::Texture* MaterialLoader::GetOrAllocateTexture(const std::string& relativePath)
    {
        auto textureIt = mMaterialTextures.find(relativePath);

        if (textureIt != mMaterialTextures.end())
        {
            return textureIt->second.get();
        }
        else
        {
            auto [iter, success] = mMaterialTextures.emplace(relativePath, mResourceLoader.LoadTexture(relativePath));
            return iter->second.get();
        }
    }

    Memory::Texture* MaterialLoader::AllocateAndStoreTexture(const HAL::Texture::Properties& properties, const std::string& relativePath)
    {
        auto [iter, success] = mMaterialTextures.emplace(relativePath, mResourceProducer->NewTexture(properties));
        return iter->second.get();
    }

}
