#pragma once

#include "ResourceDescriptorStorage.hpp"

#include <memory>
#include <vector>

namespace PathFinder
{

    class AssetResourceStorage
    {
    public:
        AssetResourceStorage(ResourceDescriptorStorage* descriptorStorage);

        uint64_t StoreAsset(std::unique_ptr<HAL::TextureResource> resource);

    private:
        ResourceDescriptorStorage* mDescriptorStorage;
        std::vector<std::unique_ptr<HAL::TextureResource>> mAssets;
    };

}
