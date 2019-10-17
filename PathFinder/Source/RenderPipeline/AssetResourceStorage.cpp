#include "AssetResourceStorage.hpp"

namespace PathFinder
{

    AssetResourceStorage::AssetResourceStorage(ResourceDescriptorStorage* descriptorStorage)
        : mDescriptorStorage{ descriptorStorage } {}

    uint64_t AssetResourceStorage::StoreAsset(std::unique_ptr<HAL::TextureResource> resource)
    {
        mAssets.push_back(std::move(resource));
        return mDescriptorStorage->EmplaceSRDescriptorIfNeeded(mAssets.back().get()).IndexInHeapRange();
    }

}
