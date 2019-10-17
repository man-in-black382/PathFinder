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
            material.AlbedoMapIndex = mAssetStorage->StoreAsset(std::move(texture));
        }
            
        if (auto texture = mTextureLoader.Load(normalMapRelativePath)) {
            material.NormalMapIndex = mAssetStorage->StoreAsset(std::move(texture));
        }

        if (auto texture = mTextureLoader.Load(roughnessMapRelativePath)) {
            material.RoughnessMapIndex = mAssetStorage->StoreAsset(std::move(texture));
        }

        if (auto texture = mTextureLoader.Load(metalnessMapRelativePath)) {
            material.MetalnessMapIndex = mAssetStorage->StoreAsset(std::move(texture));
        }

        if (AOMapRelativePath; auto texture = mTextureLoader.Load(*AOMapRelativePath)) {
            material.AOMapIndex = mAssetStorage->StoreAsset(std::move(texture));
        }

        return material;
    }

}
