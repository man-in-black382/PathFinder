#pragma once

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"
#include "../HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp"

#include "../Scene/Mesh.hpp"
#include "../Scene/MeshInstance.hpp"
#include "../Scene/Vertices/Vertex1P1N1UV1T1BT.hpp"
#include "../Scene/Vertices/Vertex1P1N1UV.hpp"
#include "../Scene/Vertices/Vertex1P3.hpp"

#include "VertexStorageLocation.hpp"
#include "VertexLayouts.hpp"
#include "CopyDevice.hpp"

#include <unordered_map>
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

        const HAL::VertexBufferDescriptor* UnifiedVertexBufferDescriptorForLayout(VertexLayout layout) const;
        const HAL::IndexBufferDescriptor* UnifiedIndexBufferDescriptorForLayout(VertexLayout layout) const;

        void FinalizeGeometryBuffers();

    private:

        template <class Vertex>
        struct UploadBufferPackage
        {
            std::vector<Vertex> Vertices;
            std::vector<uint32_t> Indices;
        };

        template <class Vertex>
        struct FinalBufferPackage
        {
            std::unique_ptr<HAL::BufferResource<Vertex>> VertexBuffer;
            std::unique_ptr<HAL::BufferResource<uint32_t>> IndexBuffer;
            std::unique_ptr<HAL::VertexBufferDescriptor> VertexBufferDescriptor;
            std::unique_ptr<HAL::IndexBufferDescriptor> IndexBufferDescriptor;
        };

        template <class Vertex>
        void CopyBuffersToDefaultHeap();

        template <class Vertex>
        VertexStorageLocation WriteToUploadBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices = nullptr, uint32_t indexCount = 0);

        std::tuple<UploadBufferPackage<Vertex1P1N1UV1T1BT>, UploadBufferPackage<Vertex1P1N1UV>, UploadBufferPackage<Vertex1P3>> mUploadBuffers;
        std::tuple<FinalBufferPackage<Vertex1P1N1UV1T1BT>, FinalBufferPackage<Vertex1P1N1UV>, FinalBufferPackage<Vertex1P3>> mFinalBuffers;

        std::unordered_map<const Mesh*, HAL::RayTracingBottomAccelerationStructure> mBottomAccelerationStructures;

        HAL::Device* mDevice;
        CopyDevice* mCopyDevice;
    };

}
