#pragma once

#include "ResourceDescriptorStorage.hpp"
#include "VertexStorageLocation.hpp"

#include "../Scene/MeshInstance.hpp"
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
        Material InstanceMaterial;
        uint32_t UnifiedVertexBufferOffset;
        uint32_t UnifiedIndexBufferOffset;
        uint32_t IndexCount;
    };

    using GPUInstanceIndex = uint64_t;
    using GPUDescriptorIndex = uint64_t;

    class AssetResourceStorage
    {
    public:
        AssetResourceStorage(const HAL::Device* device, ResourceDescriptorStorage* descriptorStorage, uint8_t simultaneousFramesInFlight);

        GPUDescriptorIndex StoreAsset(std::unique_ptr<HAL::TextureResource> resource);
        GPUInstanceIndex StoreMeshInstance(const MeshInstance& instance, const HAL::RayTracingBottomAccelerationStructure& blas);

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

        void ResetInstanceStorages();
        void AllocateTopAccelerationStructureIfNeeded();

    private:
        ResourceDescriptorStorage* mDescriptorStorage;
        std::vector<std::unique_ptr<HAL::TextureResource>> mAssets;
        HAL::RingBufferResource<GPUInstanceTableEntry> mInstanceTable;
        HAL::RayTracingTopAccelerationStructure mTopAccelerationStructure;
        HAL::ResourceBarrierCollection mTopAccelerationStructureBarriers;
        uint64_t mCurrentFrameInsertedInstanceCount = 0;

    public:
        const auto& InstanceTable() const { return mInstanceTable; }
        const auto& TopAccelerationStructure() const { return mTopAccelerationStructure; }
        const auto& TopAccelerationStructureUABarriers() const { return mTopAccelerationStructureBarriers; }
    };

}
