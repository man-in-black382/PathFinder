#include "MaterialLoader.hpp"

namespace PathFinder
{
    
    MaterialLoader::MaterialLoader(const std::filesystem::path& fileRoot, const HAL::Device* device, AssetResourceStorage* assetStorage, CopyDevice* copyDevice)
        :  mAssetStorage{ assetStorage }, mTextureLoader{ fileRoot, device, copyDevice } {}

    Material MaterialLoader::LoadMaterial(
        const std::string& albedoMapRelativePath, 
        const std::string& normalMapRelativePath, 
        const std::string& roughnessMapRelativePath, 
        const std::string& metalnessMapRelativePath,
        std::optional<std::string> AOMapRelativePath)
    {
        Material material{};

        if (auto texture = mTextureLoader.Load(albedoMapRelativePath)) {
            material.AlbedoMapSRVIndex = mAssetStorage->StoreAsset(std::move(texture));
        }
            
        if (auto texture = mTextureLoader.Load(normalMapRelativePath)) {
            material.NormalMapSRVIndex = mAssetStorage->StoreAsset(std::move(texture));
        }

        if (auto texture = mTextureLoader.Load(roughnessMapRelativePath)) {
            material.RoughnessMapSRVIndex = mAssetStorage->StoreAsset(std::move(texture));
        }

        if (auto texture = mTextureLoader.Load(metalnessMapRelativePath)) {
            material.MetalnessMapSRVIndex = mAssetStorage->StoreAsset(std::move(texture));
        }

        if (AOMapRelativePath; auto texture = mTextureLoader.Load(*AOMapRelativePath)) {
            material.AOMapSRVIndex = mAssetStorage->StoreAsset(std::move(texture));
        }

        return material;
    }

}
