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

        if (auto texture = mTextureLoader.Load(albedoMapRelativePath)) {
            material.AlbedoMapSRVIndex = mAssetStorage->StoreAsset(texture);
        }
            
        if (auto texture = mTextureLoader.Load(normalMapRelativePath)) {
            material.NormalMapSRVIndex = mAssetStorage->StoreAsset(texture);
        }

        if (auto texture = mTextureLoader.Load(roughnessMapRelativePath)) {
            material.RoughnessMapSRVIndex = mAssetStorage->StoreAsset(texture);
        }

        if (auto texture = mTextureLoader.Load(metalnessMapRelativePath)) {
            material.MetalnessMapSRVIndex = mAssetStorage->StoreAsset(texture);
        }

        if (displacementMapRelativePath; auto texture = mTextureLoader.Load(*displacementMapRelativePath))
        {
            material.DisplacementMapSRVIndex = mAssetStorage->StoreAsset(texture);

            auto distanceMap = std::make_unique<HAL::TextureResource>(
                *mDevice, HAL::ResourceFormat::Color::RGBA16_Float, HAL::ResourceFormat::TextureKind::Texture3D,
                Geometry::Dimensions{ 128, 128, 8 }, HAL::ResourceFormat::ColorClearValue{ 0.0, 0.0, 0.0, 0.0 },
                HAL::ResourceState::UnorderedAccess,
                HAL::ResourceState::CopySource | HAL::ResourceState::PixelShaderAccess | HAL::ResourceState::NonPixelShaderAccess);

            auto distanceMapPreprocessableAsset = mAssetStorage->StoreAndPreprocessAsset(std::move(distanceMap), true);

            material.DistanceMapSRVIndex = distanceMapPreprocessableAsset.SRIndex;
            material.DistanceMapUAVIndex = distanceMapPreprocessableAsset.UAIndex;
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
