#include "MaterialLoader.hpp"

namespace PathFinder
{

    MaterialLoader::MaterialLoader(const std::filesystem::path& fileRoot, PreprocessableAssetStorage* assetStorage, Memory::GPUResourceProducer* resourceProducer)
        : mAssetStorage{ assetStorage }, mResourceLoader{ fileRoot, resourceProducer }, mResourceProducer{ resourceProducer } {}

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

        material.AlbedoMap = mResourceLoader.LoadTexture(albedoMapRelativePath);
        material.NormalMap = mResourceLoader.LoadTexture(normalMapRelativePath);
        material.RoughnessMap = mResourceLoader.LoadTexture(roughnessMapRelativePath);
        material.MetalnessMap = mResourceLoader.LoadTexture(metalnessMapRelativePath);

        if (displacementMapRelativePath)
        {
            material.DisplacementMap = mResourceLoader.LoadTexture(*displacementMapRelativePath);
        }

        if (AOMapRelativePath)
        {
            material.AOMap = mResourceLoader.LoadTexture(*AOMapRelativePath);
        }

        if (material.DisplacementMap && distanceFieldRelativePath)
        {
            material.DistanceField = mResourceLoader.LoadTexture(*distanceFieldRelativePath);

            if (!material.DistanceField)
            {
                HAL::Texture::Properties distFieldProperties{
                    HAL::ColorFormat::RGBA32_Unsigned, HAL::TextureKind::Texture3D,
                    DistanceFieldTextureSize, HAL::ResourceState::UnorderedAccess, HAL::ResourceState::AnyShaderAccess };

                material.DistanceField = mResourceProducer->NewTexture(distFieldProperties);

                mAssetStorage->PreprocessAsset(material.DistanceField.get(), [this, distanceFieldRelativePath](Memory::GPUResource* distanceField)
                {
                    mResourceLoader.StoreResource(*distanceField, *distanceFieldRelativePath);
                });
            }
        }

        return std::move(material);
    }

}
