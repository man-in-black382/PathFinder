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
        return {};
    }

}
