#include "AssetResourceStorage.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    AssetResourceStorage::AssetResourceStorage(const HAL::Device* device, ResourceDescriptorStorage* descriptorStorage, uint8_t simultaneousFramesInFlight)
        : mDescriptorStorage{ descriptorStorage }, 
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

    GPUDescriptorIndex AssetResourceStorage::StoreAsset(std::unique_ptr<HAL::TextureResource> resource)
    {
        mAssets.push_back(std::move(resource));
        return mDescriptorStorage->EmplaceSRDescriptorIfNeeded(mAssets.back().get()).IndexInHeapRange();
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
