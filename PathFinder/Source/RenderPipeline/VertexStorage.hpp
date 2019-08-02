#pragma once

#include "../Scene/Mesh.hpp"
#include "../Scene/MeshInstance.hpp"
#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"
#include "../Scene/Vertices/Vertex1P1N1UV1T1BT.hpp"
#include "../Scene/Vertices/Vertex1P1N1UV.hpp"
#include "../Scene/Vertices/Vertex1P3.hpp"

#include "VertexStorageLocation.hpp"
#include "VertexLayouts.hpp"

#include <unordered_map>
#include <memory>
#include <tuple>

namespace PathFinder
{

    class VertexStorage
    {
    public:
        VertexStorage(HAL::Device* device);

        VertexStorageLocation AddVertices(const Vertex1P1N1UV1T1BT* vertices, uint64_t vertexCount, const uint32_t* indices = nullptr, uint64_t indexCount = 0);
        VertexStorageLocation AddVertices(const Vertex1P1N1UV* vertices, uint64_t vertexCount, const uint32_t* indices = nullptr, uint64_t indexCount = 0);
        VertexStorageLocation AddVertices(const Vertex1P3* vertices, uint64_t vertexCount, const uint32_t* indices = nullptr, uint64_t indexCount = 0);

        const HAL::VertexBufferDescriptor* UnifiedVertexBufferDescriptorForLayout(VertexLayout layout) const;
        const HAL::IndexBufferDescriptor* UnifiedIndexBufferDescriptorForLayout(VertexLayout layout) const;

        void TransferDataToGPU();

    private:

        template <class Vertex>
        struct UploadBufferPackage
        {
            std::unique_ptr<HAL::BufferResource<Vertex>> VertexBuffer;
            std::unique_ptr<HAL::BufferResource<uint32_t>> IndexBuffer;
            uint64_t CurrentVertexOffset = 0;
            uint64_t CurrentIndexOffset = 0;
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
        void AllocateUploadBuffersIfNeeded();

        template <class Vertex>
        void AllocateFinalBuffersIfNeeded();

        template <class Vertex>
        void CopyBuffersToDefaultHeap();

        template <class Vertex>
        VertexStorageLocation WriteToUploadBuffers(const Vertex* vertices, uint64_t vertexCount, const uint32_t* indices = nullptr, uint64_t indexCount = 0);

        uint64_t mUploadBufferCapacity = 50 * 1024 * 1024;

        std::tuple<UploadBufferPackage<Vertex1P1N1UV1T1BT>, UploadBufferPackage<Vertex1P1N1UV>, UploadBufferPackage<Vertex1P3>> mUploadBuffers;
        std::tuple<FinalBufferPackage<Vertex1P1N1UV1T1BT>, FinalBufferPackage<Vertex1P1N1UV>, FinalBufferPackage<Vertex1P3>> mFinalBuffers;

        HAL::Device* mDevice;
        HAL::DirectCommandAllocator mCommandAllocator;
        HAL::DirectCommandList mCommandList;
        HAL::DirectCommandQueue mCommandQueue;
        HAL::Fence mFence;
    };

}
