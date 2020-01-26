namespace PathFinder
{

    //template <class T>
    //PreprocessableAsset<T> AssetResourceStorage::StorePreprocessableAsset(std::shared_ptr<HAL::Buffer<T>> asset, bool queueContentReadback)
    //{
    //    mAssets.push_back(asset);

    //    auto srIndex = mDescriptorStorage->EmplaceSRDescriptorIfNeeded(asset.get()).IndexInHeapRange();
    //    auto uaIndex = mDescriptorStorage->EmplaceUADescriptorIfNeeded(asset.get()).IndexInHeapRange();

    //    std::shared_ptr<HAL::Buffer<T>> readBackBuffer = nullptr;

    //    if (queueContentReadback)
    //    {
    //        readBackBuffer = mCopyDevice->QueueResourceCopyToReadbackMemory(asset);
    //    }

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
