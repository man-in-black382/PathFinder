#include "AssetResourceStorage.hpp"

#include "../Foundation/Assert.hpp"
#include "../HardwareAbstractionLayer/ResourceFootprint.hpp"

namespace PathFinder
{

    AssetResourceStorage::AssetResourceStorage(const HAL::Device* device, CopyDevice* copyDevice, ResourceDescriptorStorage* descriptorStorage, uint8_t simultaneousFramesInFlight)
        : mDevice{ device },
        mCopyDevice{ copyDevice },
        mDescriptorStorage{ descriptorStorage }, 
        mInstanceTable{ *device, 1024, simultaneousFramesInFlight, 256, HAL::CPUAccessibleHeapType::Upload },
        mTopAccelerationStructure{ device } {}

    void AssetResourceStorage::BeginFrame(uint64_t frameFenceValue)
    {
        mInstanceTable.PrepareMemoryForNewFrame(frameFenceValue);
    }

    void AssetResourceStorage::EndFrame(uint64_t completedFrameFenceValue)
    {
        mInstanceTable.DiscardMemoryForCompletedFrames(completedFrameFenceValue);
    }

    void AssetResourceStorage::ResetInstanceStorages()
    {
        // Simply reset an index into the instance ring buffer
        mCurrentFrameInsertedInstanceCount = 0;

        // Clear inputs of RT acceleration structure
        mTopAccelerationStructure.ResetInputs();
    }

    void AssetResourceStorage::AllocateTopAccelerationStructureIfNeeded()
    {
        mTopAccelerationStructure.AllocateBuffersIfNeeded();
        mTopAccelerationStructure.SetDebugName("Unified_Top_Acceleration_Structure");
        mTopAccelerationStructureBarriers = HAL::ResourceBarrierCollection{};
        mTopAccelerationStructureBarriers.AddBarrier(HAL::UnorderedAccessResourceBarrier{ mTopAccelerationStructure.FinalBuffer() });
    }

    GPUDescriptorIndex AssetResourceStorage::StoreAsset(std::shared_ptr<HAL::TextureResource> resource)
    {
        mAssets.push_back(resource);
        return mDescriptorStorage->EmplaceSRDescriptorIfNeeded(resource.get()).IndexInHeapRange();
    }

    PreprocessableAsset<uint8_t> AssetResourceStorage::StorePreprocessableAsset(std::shared_ptr<HAL::TextureResource> asset, bool queueContentReadback)
    {        
        mAssets.push_back(asset);
        
        auto srIndex = mDescriptorStorage->EmplaceSRDescriptorIfNeeded(asset.get()).IndexInHeapRange();
        auto uaIndex = mDescriptorStorage->EmplaceUADescriptorIfNeeded(asset.get()).IndexInHeapRange();

        std::shared_ptr<HAL::BufferResource<uint8_t>> readBackBuffer = nullptr;

        if (queueContentReadback)
        {
            readBackBuffer = mCopyDevice->QueueResourceCopyToReadbackMemory(asset);
        }

        mAssetPostProcessingBarriers.AddBarrier(
            HAL::ResourceTransitionBarrier{
                asset->InitialStates(),
                HAL::ResourceState::Common, // Prepare to be used on copy queue 
                asset.get()
            }
        );

        return { srIndex, uaIndex, std::move(readBackBuffer) };
    }

    GPUInstanceIndex AssetResourceStorage::StoreMeshInstance(const MeshInstance& instance, const HAL::RayTracingBottomAccelerationStructure& blas)
    {
        assert_format(mCurrentFrameInsertedInstanceCount < mInstanceTable.PerFrameCapacity(),
            "Buffer capacity is static in current implementation and you have reached a maximum amount of updates per frame");

        GPUInstanceTableEntry instanceEntry{
            instance.Transformation().ModelMatrix(),
            instance.Transformation().NormalMatrix(),
            instance.AssosiatedMaterial()->AlbedoMapSRVIndex,
            instance.AssosiatedMaterial()->NormalMapSRVIndex,
            instance.AssosiatedMaterial()->RoughnessMapSRVIndex,
            instance.AssosiatedMaterial()->MetalnessMapSRVIndex,
            instance.AssosiatedMaterial()->AOMapSRVIndex,
            instance.AssosiatedMaterial()->DisplacementMapSRVIndex,
            instance.AssosiatedMesh()->LocationInVertexStorage().VertexBufferOffset,
            instance.AssosiatedMesh()->LocationInVertexStorage().IndexBufferOffset,
            instance.AssosiatedMesh()->LocationInVertexStorage().IndexCount
        };

        GPUInstanceIndex instanceIndex = mCurrentFrameInsertedInstanceCount;

        mInstanceTable.Write(mCurrentFrameInsertedInstanceCount, &instanceEntry);
        ++mCurrentFrameInsertedInstanceCount;

        mTopAccelerationStructure.AddInstance(blas, instanceIndex, instance.Transformation().ModelMatrix());

        return instanceIndex;
    }

}
