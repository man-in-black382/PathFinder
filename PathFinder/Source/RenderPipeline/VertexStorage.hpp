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

    class VertexStorage
    {
    public:
        VertexStorage(HAL::Device* device, CopyDevice* copyDevice);

        VertexStorageLocation AddVertices(const Vertex1P1N1UV1T1BT* vertices, uint32_t vertexCount, const uint32_t* indices = nullptr, uint32_t indexCount = 0);
        VertexStorageLocation AddVertices(const Vertex1P1N1UV* vertices, uint32_t vertexCount, const uint32_t* indices = nullptr, uint32_t indexCount = 0);
        VertexStorageLocation AddVertices(const Vertex1P3* vertices, uint32_t vertexCount, const uint32_t* indices = nullptr, uint32_t indexCount = 0);

        const HAL::BufferResource<Vertex1P1N1UV1T1BT>* UnifiedVertexBuffer_1P1N1UV1T1BT() const;
        const HAL::BufferResource<Vertex1P1N1UV>* UnifiedVertexBuffer_1P1N1UV() const;
        const HAL::BufferResource<Vertex1P3>* UnifiedVertexBuffer_1P() const;

        const HAL::BufferResource<uint32_t>* UnifiedIndexBuffer_1P1N1UV1T1BT() const;
        const HAL::BufferResource<uint32_t>* UnifiedIndexBuffer_1P1N1UV() const;
        const HAL::BufferResource<uint32_t>* UnifiedIndexBuffer_1P() const;

        // Barriers to wait for AS build completion
        const HAL::ResourceBarrierCollection& AccelerationStructureBarriers() const;

        void AllocateAndQueueBuffersForCopy();

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
        VertexStorageLocation WriteToUploadBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices = nullptr, uint32_t indexCount = 0);

        std::tuple<UploadBufferPackage<Vertex1P1N1UV1T1BT>, UploadBufferPackage<Vertex1P1N1UV>, UploadBufferPackage<Vertex1P3>> mUploadBuffers;
        std::tuple<FinalBufferPackage<Vertex1P1N1UV1T1BT>, FinalBufferPackage<Vertex1P1N1UV>, FinalBufferPackage<Vertex1P3>> mFinalBuffers;

        std::vector<HAL::RayTracingBottomAccelerationStructure> mBottomAccelerationStructures;
        HAL::ResourceBarrierCollection mAccelerationStructureBarriers;

        HAL::Device* mDevice;
        CopyDevice* mCopyDevice;

    public:
        inline const auto& BottomAccelerationStructures() const { return mBottomAccelerationStructures; }
    };

}
