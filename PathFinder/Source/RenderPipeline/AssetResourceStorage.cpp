#include "AssetResourceStorage.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    AssetResourceStorage::AssetResourceStorage(const HAL::Device& device, ResourceDescriptorStorage* descriptorStorage, uint8_t simultaneousFramesInFlight)
        : mDescriptorStorage{ descriptorStorage }, 
        mInstanceTable{ device, 1024, simultaneousFramesInFlight, 256, HAL::CPUAccessibleHeapType::Upload } {}

    uint64_t AssetResourceStorage::StoreAsset(std::unique_ptr<HAL::TextureResource> resource)
    {
        mAssets.push_back(std::move(resource));
        return mDescriptorStorage->EmplaceSRDescriptorIfNeeded(mAssets.back().get()).IndexInHeapRange();
    }

    void AssetResourceStorage::BeginFrame(uint64_t frameFenceValue)
    {
        mInstanceTable.PrepareMemoryForNewFrame(frameFenceValue);
        mCurrentFrameInsertedInstanceCount = 0;
    }

    void AssetResourceStorage::EndFrame(uint64_t completedFrameFenceValue)
    {
        mInstanceTable.DiscardMemoryForCompletedFrames(completedFrameFenceValue);
    }

    uint64_t AssetResourceStorage::StoreInstanceData(const GPUInstanceTableEntry& instanceData)
    {
        assert_format(mCurrentFrameInsertedInstanceCount < mInstanceTable.PerFrameCapacity(),
            "Buffer capacity is static in current implementation and you have reached a maximum amount of updates per frame");

        mInstanceTable.Write(mCurrentFrameInsertedInstanceCount, &instanceData);
        ++mCurrentFrameInsertedInstanceCount;

        return 0;
    }

}
