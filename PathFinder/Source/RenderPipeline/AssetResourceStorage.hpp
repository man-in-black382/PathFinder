#pragma once

#include "ResourceDescriptorStorage.hpp"
#include "VertexStorageLocation.hpp"

#include "../Scene/MeshInstance.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"
#include "../HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp"

#include <memory>
#include <vector>

namespace PathFinder
{

    class AssetResourceStorage
    {
    public:
        AssetResourceStorage(const HAL::Device& device, ResourceDescriptorStorage* descriptorStorage, uint8_t simultaneousFramesInFlight);

        uint64_t StoreAsset(std::unique_ptr<HAL::TextureResource> resource);
        uint64_t UpdateInstanceTable(const GPUInstanceTableEntry& instanceData);

        //void CreateRayTracingBottomAccelerationStructure()

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

    private:
        ResourceDescriptorStorage* mDescriptorStorage;
        std::vector<std::unique_ptr<HAL::TextureResource>> mAssets;
        HAL::RingBufferResource<GPUInstanceTableEntry> mInstanceTable;
        uint64_t mCurrentFrameInsertedInstanceCount = 0;

    public:
        const auto& InstanceTable() const { return mInstanceTable; }
    };

}
