#pragma once

#include "../Scene/Mesh.hpp"
#include "../Scene/MeshInstance.hpp"
#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"

#include <unordered_map>
#include <memory>

namespace PathFinder
{

    class MeshGPUStorage
    {
    public:
        MeshGPUStorage(HAL::Device* device);

        MeshInstance EmplaceInstanceForMesh(const Mesh* mesh);
        void TransferDataToGPU();

    private:
        uint64_t mUploadBufferCapacity = 50 * 1024 * 1024;
        uint64_t mCurrentVertexOffset = 0;
        uint64_t mCurrentIndexOffset = 0;

        std::unordered_map<const Mesh *, std::vector<MeshInstance::GPUBufferLocation>> mEmplacedMeshBufferLocations;

        HAL::Device* mDevice;
        HAL::DirectCommandAllocator mCommandAllocator;
        HAL::DirectCommandList mCommandList;
        HAL::DirectCommandQueue mCommandQueue;
        HAL::Fence mFence;

        std::unique_ptr<HAL::BufferResource<Vertex1P1N1UV1T1BT>> mUploadVertexBuffer;
        std::unique_ptr<HAL::BufferResource<uint32_t>> mUploadIndexBuffer;
        
        std::unique_ptr<HAL::BufferResource<Vertex1P1N1UV1T1BT>> mFinalVertexBuffer;
        std::unique_ptr<HAL::BufferResource<uint32_t>> mFinalIndexBuffer;

        std::unique_ptr<HAL::VertexBufferDescriptor> mVertexBufferDescriptor;
        std::unique_ptr<HAL::IndexBufferDescriptor> mIndexBufferDescriptor;

    public:
        const HAL::BufferResource<Vertex1P1N1UV1T1BT>* UnifiedVertexBuffer() const { return mFinalVertexBuffer.get(); }
        const HAL::BufferResource<uint32_t>* UnifiedIndexBuffer() const { return mFinalIndexBuffer.get(); }
        const HAL::VertexBufferDescriptor* UnifiedVertexBufferDescriptor() const { return mVertexBufferDescriptor.get(); }
        const HAL::IndexBufferDescriptor* UnifiedIndexBufferDescriptor() const { return mIndexBufferDescriptor.get(); }
    };

}
