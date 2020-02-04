#include "AssetResourceStorage.hpp"

#include "../Foundation/Assert.hpp"
#include "../HardwareAbstractionLayer/ResourceFootprint.hpp"

namespace PathFinder
{

    AssetResourceStorage::AssetResourceStorage(const HAL::Device* device, ResourceDescriptorStorage* descriptorStorage)
        : mDevice{ device }, mDescriptorStorage{ descriptorStorage } {}

    GPUDescriptorIndex AssetResourceStorage::StoreAsset(std::shared_ptr<HAL::Texture> resource)
    {
        mAssets.push_back(resource);
        return mDescriptorStorage->EmplaceSRDescriptorIfNeeded(resource.get()).IndexInHeapRange();
    }

    //PreprocessableAsset<uint8_t> AssetResourceStorage::StorePreprocessableAsset(std::shared_ptr<HAL::Texture> asset, bool queueContentReadback)
    //{        
    //    mAssets.push_back(asset);
    //    
    //    auto srIndex = mDescriptorStorage->EmplaceSRDescriptorIfNeeded(asset.get()).IndexInHeapRange();
    //    auto uaIndex = mDescriptorStorage->EmplaceUADescriptorIfNeeded(asset.get()).IndexInHeapRange();

    //    std::shared_ptr<HAL::Buffer<uint8_t>> readBackBuffer = nullptr;

    //    if (queueContentReadback)
    //    {
    //        readBackBuffer = mCopyDevice->QueueResourceCopyToReadbackMemory(asset);
    //    }

    //    mAssetPostProcessingBarriers.AddBarrier(HAL::UnorderedAccessResourceBarrier{ asset.get() });

    //    mAssetPostProcessingBarriers.AddBarrier(
    //        HAL::ResourceTransitionBarrier{
    //            asset->InitialStates(),
    //            HAL::ResourceState::Common, // Prepare to be used on copy queue 
    //            asset.get()
    //        }
    //    );

    //    return { srIndex, uaIndex, std::move(readBackBuffer) };
    //}

}
