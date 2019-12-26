#pragma once

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"
#include "../HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp"
#include "../HardwareAbstractionLayer/ResourceBarrier.hpp"

#include "../Scene/Mesh.hpp"
#include "../Scene/MeshInstance.hpp"
#include "../Scene/Vertices/Vertex1P1N1UV1T1BT.hpp"
#include "../Scene/Vertices/Vertex1P1N1UV.hpp"
#include "../Scene/Vertices/Vertex1P3.hpp"

#include "VertexStorageLocation.hpp"
#include "VertexLayouts.hpp"
#include "CopyDevice.hpp"

#include <vector>
#include <memory>
#include <tuple>

namespace PathFinder
{

    struct GPUInstanceTableEntry
    {
        glm::mat4 InstanceWorldMatrix;
        glm::mat4 InstanceNormalMatrix;
        uint32_t AlbedoMapIndex;
        uint32_t NormalMapIndex;
        uint32_t RoughnessMapIndex;
        uint32_t MetalnessMapIndex;
        uint32_t AOMapIndex;
        uint32_t DisplacementMapIndex;
        uint32_t DistanceAtlasIndirectionMapIndex;
        uint32_t DistanceAtlasIndex;
        uint32_t UnifiedVertexBufferOffset;
        uint32_t UnifiedIndexBufferOffset;
        uint32_t IndexCount;
    };

    using GPUInstanceIndex = uint64_t;

    class MeshGPUStorage
    {
    public:
        MeshGPUStorage(HAL::Device* device, CopyDevice* copyDevice, uint8_t simultaneousFramesInFlight);

        void StoreMesh(Mesh& mesh);
        void StoreMeshInstance(MeshInstance& instance, const HAL::RayTracingBottomAccelerationStructure& blas);

        const HAL::BufferResource<Vertex1P1N1UV1T1BT>* UnifiedVertexBuffer_1P1N1UV1T1BT() const;
        const HAL::BufferResource<Vertex1P1N1UV>* UnifiedVertexBuffer_1P1N1UV() const;
        const HAL::BufferResource<Vertex1P3>* UnifiedVertexBuffer_1P() const;

        const HAL::BufferResource<uint32_t>* UnifiedIndexBuffer_1P1N1UV1T1BT() const;
        const HAL::BufferResource<uint32_t>* UnifiedIndexBuffer_1P1N1UV() const;
        const HAL::BufferResource<uint32_t>* UnifiedIndexBuffer_1P() const;

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

        void UploadVerticesAndQueueForCopy();
        void CreateBottomAccelerationStructures();
        void CreateTopAccelerationStructure();

    private:
        template <class Vertex>
        struct UploadBufferPackage
        {
            std::vector<Vertex> Vertices;
            std::vector<uint32_t> Indices;
            std::vector<VertexStorageLocation> Locations;
        };

        template <class Vertex>
        struct FinalBufferPackage
        {
            std::shared_ptr<HAL::BufferResource<Vertex>> VertexBuffer;
            std::shared_ptr<HAL::BufferResource<uint32_t>> IndexBuffer;
        };

        template <class Vertex>
        void CopyBuffersToDefaultHeap();

        template <class Vertex>
        void CreateBottomAccelerationStructuresInternal();

        template <class Vertex>
        VertexStorageLocation WriteToUploadBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices = nullptr, uint32_t indexCount = 0);

        std::tuple<UploadBufferPackage<Vertex1P1N1UV1T1BT>, UploadBufferPackage<Vertex1P1N1UV>, UploadBufferPackage<Vertex1P3>> mUploadBuffers;
        std::tuple<FinalBufferPackage<Vertex1P1N1UV1T1BT>, FinalBufferPackage<Vertex1P1N1UV>, FinalBufferPackage<Vertex1P3>> mFinalBuffers;

        std::vector<HAL::RayTracingBottomAccelerationStructure> mBottomAccelerationStructures;
        HAL::RayTracingTopAccelerationStructure mTopAccelerationStructure;
        HAL::ResourceBarrierCollection mTopASBuildBarriers;
        HAL::ResourceBarrierCollection mBottomASBuildBarriers;
        HAL::RingBufferResource<GPUInstanceTableEntry> mInstanceTable;
        
        uint64_t mCurrentFrameInsertedInstanceCount = 0;

        HAL::Device* mDevice;
        CopyDevice* mCopyDevice;

    public:
        inline const auto& InstanceTable() const { return mInstanceTable; }
        inline const auto& TopAccelerationStructure() const { return mTopAccelerationStructure; }
        inline const auto& BottomAccelerationStructures() const { return mBottomAccelerationStructures; }
        inline const auto& TopASBuildBarriers() const { return mTopASBuildBarriers; }
        inline const auto& BottomASBuildBarriers() const { return mBottomASBuildBarriers; }
    };

}
