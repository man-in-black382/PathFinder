#include "MaterialLoader.hpp"

namespace PathFinder
{
    
    MaterialLoader::MaterialLoader(const std::filesystem::path& fileRoot, const HAL::Device* device, AssetResourceStorage* assetStorage, CopyDevice* copyDevice)
        : mDevice{ device }, mAssetStorage{ assetStorage }, mTextureLoader{ fileRoot, device, copyDevice } {}

    Material MaterialLoader::LoadMaterial(
        const std::string& albedoMapRelativePath, 
        const std::string& normalMapRelativePath, 
        const std::string& roughnessMapRelativePath, 
        const std::string& metalnessMapRelativePath,
        std::optional<std::string> displacementMapRelativePath,
        std::optional<std::string> distanceMapRelativePath,
        std::optional<std::string> AOMapRelativePath)
    {
        Material material{};

        if (auto texture = mTextureLoader.Load(albedoMapRelativePath)) 
        {
            material.AlbedoMap = texture.get();
            material.AlbedoMapSRVIndex = mAssetStorage->StoreAsset(texture);
        }
            
        if (auto texture = mTextureLoader.Load(normalMapRelativePath)) 
        {
            material.NormalMap = texture.get();
            material.NormalMapSRVIndex = mAssetStorage->StoreAsset(texture);
        }

        if (auto texture = mTextureLoader.Load(roughnessMapRelativePath)) 
        {
            material.RoughnessMap = texture.get();
            material.RoughnessMapSRVIndex = mAssetStorage->StoreAsset(texture);
        }

        if (auto texture = mTextureLoader.Load(metalnessMapRelativePath)) 
        {
            material.MetalnessMap = texture.get();
            material.MetalnessMapSRVIndex = mAssetStorage->StoreAsset(texture);
        }

        if (displacementMapRelativePath; auto texture = mTextureLoader.Load(*displacementMapRelativePath))
        {
            material.DisplacementMap = texture.get();
            material.DisplacementMapSRVIndex = mAssetStorage->StoreAsset(texture);

            if (distanceMapRelativePath; auto texture = mTextureLoader.Load(*distanceMapRelativePath))
            {
                // Load from file
            }
            else {
                auto iStates = HAL::ResourceState::UnorderedAccess;
                auto eStates = HAL::ResourceState::CopySource | HAL::ResourceState::PixelShaderAccess | HAL::ResourceState::NonPixelShaderAccess;

                auto distanceIndirectionMap = std::make_shared<HAL::TextureResource>(
                    *mDevice, HAL::ResourceFormat::Color::R16_Float, HAL::ResourceFormat::TextureKind::Texture3D,
                    Geometry::Dimensions{ 128, 128, 64 }, HAL::ResourceFormat::ColorClearValue{ 0.0, 0.0, 0.0, 0.0 }, iStates, eStates);

                auto distanceAtlas = std::make_shared<HAL::TextureResource>(
                    *mDevice, HAL::ResourceFormat::Color::R16_Float, HAL::ResourceFormat::TextureKind::Texture3D,
                    Geometry::Dimensions{ 128, 128, 8 }, HAL::ResourceFormat::ColorClearValue{ 0.0, 0.0, 0.0, 0.0 }, iStates, eStates);

                auto atlasEntryCounter = std::make_shared<HAL::BufferResource<uint32_t>>(
                    *mDevice, 1, 1, HAL::ResourceState::UnorderedAccess, HAL::ResourceState::CopySource);

                distanceIndirectionMap->SetDebugName("DistanceIndirectionMap");
                distanceAtlas->SetDebugName("DistanceAtlas");
                atlasEntryCounter->SetDebugName("DistanceAtlasCounter");

                auto distanceIndirectionAsset = mAssetStorage->StorePreprocessableAsset(distanceIndirectionMap, true);
                auto distanceAtlasAsset = mAssetStorage->StorePreprocessableAsset(distanceAtlas, true);
                auto counterAsset = mAssetStorage->StorePreprocessableAsset(atlasEntryCounter, true);

                material.DistanceAtlasIndirectionMapSRVIndex = distanceIndirectionAsset.SRIndex;
                material.DistanceAtlasSRVIndex = distanceAtlasAsset.SRIndex;

                material.DistanceAtlasIndirectionMap = distanceIndirectionMap.get();
                material.DistanceAtlas = distanceAtlas.get();
                material.DistanceAtlasCounter = atlasEntryCounter.get();
            }
        }

        if (AOMapRelativePath; auto texture = mTextureLoader.Load(*AOMapRelativePath)) {
            material.AOMapSRVIndex = mAssetStorage->StoreAsset(texture);
        }

        return material;
    }

    void MaterialLoader::SerializePostprocessedTextures()
    {

    }

}
