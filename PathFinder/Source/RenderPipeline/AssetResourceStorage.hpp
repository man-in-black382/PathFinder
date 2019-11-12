#pragma once

#include "ResourceDescriptorStorage.hpp"
#include "VertexStorageLocation.hpp"
#include "CopyDevice.hpp"

#include "../Scene/MeshInstance.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"
#include "../HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp"
#include "../HardwareAbstractionLayer/ResourceBarrier.hpp"

#include <memory>
#include <vector>

namespace PathFinder
{

    struct GPUInstanceTableEntry
    {
        glm::mat4 InstanceWorldMatrix;
        glm::mat4 InstanceNormalMatrix;
        uint32_t AlbedoMapSRVIndex;
        uint32_t NormalMapSRVIndex;
        uint32_t RoughnessMapSRVIndex;
        uint32_t MetalnessMapSRVIndex;
        uint32_t AOMapSRVIndex;
        uint32_t UnifiedVertexBufferOffset;
        uint32_t UnifiedIndexBufferOffset;
        uint32_t IndexCount;
    };

    using GPUInstanceIndex = uint64_t;
    using GPUDescriptorIndex = uint64_t;

    struct PreprocessableAsset
    {
        GPUDescriptorIndex SRIndex = 0;
        GPUDescriptorIndex UAIndex = 0;
        std::shared_ptr<HAL::BufferResource<uint8_t>> AssetReadbackBuffer = nullptr;
    };

    class AssetResourceStorage
    {
    public:
        AssetResourceStorage(const HAL::Device* device, CopyDevice* copyDevice, ResourceDescriptorStorage* descriptorStorage, uint8_t simultaneousFramesInFlight);

        GPUDescriptorIndex StoreAsset(std::shared_ptr<HAL::TextureResource> resource);

        // Store asset that is required to be processed by an asset-processing render pass.
        // When processing is done an optional transfer to a read-back buffer can be performed
        PreprocessableAsset StorePreprocessableAsset(std::unique_ptr<HAL::TextureResource> resource, bool queueContentReadback);

        GPUInstanceIndex StoreMeshInstance(const MeshInstance& instance, const HAL::RayTracingBottomAccelerationStructure& blas);

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

        void ResetInstanceStorages();
        void AllocateTopAccelerationStructureIfNeeded();

    private:
        const HAL::Device* mDevice;
        CopyDevice* mCopyDevice;
        ResourceDescriptorStorage* mDescriptorStorage;
        std::vector<std::shared_ptr<HAL::TextureResource>> mAssets;
        HAL::RingBufferResource<GPUInstanceTableEntry> mInstanceTable;
        HAL::RayTracingTopAccelerationStructure mTopAccelerationStructure;
        uint64_t mCurrentFrameInsertedInstanceCount = 0;

        // Barriers for waiting for AS build to complete
        HAL::ResourceBarrierCollection mTopAccelerationStructureBarriers;

        // Barriers for resources that were processed by asset-processing
        // render passes and need to be prepared for copies on copy device
        HAL::ResourceBarrierCollection mAssetPostProcessingBarriers;

    public:
        const auto& InstanceTable() const { return mInstanceTable; }
        const auto& TopAccelerationStructure() const { return mTopAccelerationStructure; }
        const auto& TopAccelerationStructureBarriers() const { return mTopAccelerationStructureBarriers; }
        const auto& AssetPostProcessingBarriers() const { return mAssetPostProcessingBarriers; }
    };

}
