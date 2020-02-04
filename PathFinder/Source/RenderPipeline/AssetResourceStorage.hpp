#pragma once

#include "ResourceDescriptorStorage.hpp"

#include "../HardwareAbstractionLayer/Texture.hpp"
#include "../HardwareAbstractionLayer/ResourceBarrier.hpp"

#include <memory>
#include <vector>

namespace PathFinder
{

    using GPUDescriptorIndex = uint64_t;

    template <class T>
    struct PreprocessableAsset
    {
        GPUDescriptorIndex SRIndex = 0;
        GPUDescriptorIndex UAIndex = 0;
        //std::shared_ptr<HAL::Buffer<T>> AssetReadbackBuffer = nullptr;
    };

    class AssetResourceStorage
    {
    public:
        AssetResourceStorage(const HAL::Device* device, ResourceDescriptorStorage* descriptorStorage);

        GPUDescriptorIndex StoreAsset(std::shared_ptr<HAL::Texture> resource);

        // Store asset that is required to be processed by an asset-processing render pass.
        // When processing is done an optional transfer to a read-back buffer can be performed
        //PreprocessableAsset<uint8_t> StorePreprocessableAsset(std::shared_ptr<HAL::Texture> asset, bool queueContentReadback);
        //template <class T> PreprocessableAsset<T> StorePreprocessableAsset(std::shared_ptr<HAL::Buffer<T>> asset, bool queueContentReadback);

    private:
        const HAL::Device* mDevice;
        ResourceDescriptorStorage* mDescriptorStorage;
        std::vector<std::shared_ptr<HAL::Resource>> mAssets;

        // Barriers for resources that were processed by asset-processing
        // render passes and need to be prepared for copies on copy device
        HAL::ResourceBarrierCollection mAssetPostProcessingBarriers;

    public:
        const auto& AssetPostProcessingBarriers() const { return mAssetPostProcessingBarriers; }
    };

}

#include "AssetResourceStorage.inl"