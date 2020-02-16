#pragma once

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/Buffer.hpp"
#include "../HardwareAbstractionLayer/ResourceBarrier.hpp"

#include "../Memory/GPUResourceProducer.hpp"

#include "../Scene/Mesh.hpp"
#include "../Scene/MeshInstance.hpp"
#include "../Scene/Vertices/Vertex1P1N1UV1T1BT.hpp"
#include "../Scene/Vertices/Vertex1P1N1UV.hpp"
#include "../Scene/Vertices/Vertex1P3.hpp"

#include "VertexStorageLocation.hpp"
#include "BottomRTAS.hpp"
#include "TopRTAS.hpp"

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
        uint32_t DistanceFieldIndex;
        uint32_t UnifiedVertexBufferOffset;
        uint32_t UnifiedIndexBufferOffset;
        uint32_t IndexCount;
    };

    using GPUInstanceIndex = uint64_t;

    class MeshGPUStorage
    {
    public:
        MeshGPUStorage(const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer);

        template < template < class ... > class Container, class ... Args >
        void StoreMeshes(Container<Mesh, Args...>& meshes);

        template < template < class ... > class Container, class ... Args >
        void UpdateMeshInstanceTable(Container<MeshInstance, Args...>& meshInstances);

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
            Memory::GPUResourceProducer::BufferPtr VertexBuffer;
            Memory::GPUResourceProducer::BufferPtr IndexBuffer;
        };

        template <class Vertex>
        void SubmitTemporaryBuffersToGPU();

        template <class Vertex>
        VertexStorageLocation WriteToTemporaryBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices = nullptr, uint32_t indexCount = 0);

        std::tuple<UploadBufferPackage<Vertex1P1N1UV1T1BT>, UploadBufferPackage<Vertex1P1N1UV>, UploadBufferPackage<Vertex1P3>> mUploadBuffers;
        std::tuple<FinalBufferPackage<Vertex1P1N1UV1T1BT>, FinalBufferPackage<Vertex1P1N1UV>, FinalBufferPackage<Vertex1P3>> mFinalBuffers;

        std::vector<BottomRTAS> mBottomAccelerationStructures;
        TopRTAS mTopAccelerationStructure;
        HAL::ResourceBarrierCollection mBottomASBarriers;
        HAL::ResourceBarrierCollection mTopASBarriers;

        Memory::GPUResourceProducer::BufferPtr mInstanceTable;
        
        const HAL::Device* mDevice;
        Memory::GPUResourceProducer* mResourceProducer;

    public:
        inline const auto UnifiedVertexBuffer() const { return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).VertexBuffer.get(); }
        inline const auto UnifiedIndexBuffer() const { return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).IndexBuffer.get(); }
        inline const auto InstanceTable() const { return mInstanceTable.get(); }
        inline const auto& TopAccelerationStructure() const { return mTopAccelerationStructure; }
        inline const auto& BottomAccelerationStructures() const { return mBottomAccelerationStructures; }
        inline const auto& BottomAccelerationStructureBarriers() const { return mBottomASBarriers; }
        inline const auto& TopAccelerationStructureBarriers() const { return mTopASBarriers; }
    };

}

#include "MeshGPUStorage.inl"
