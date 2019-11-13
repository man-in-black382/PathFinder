#include "VertexStorage.hpp"

#include <algorithm>
#include <iterator>

namespace PathFinder
{

    VertexStorage::VertexStorage(HAL::Device* device, CopyDevice* copyDevice)
        : mDevice{ device }, mCopyDevice{ copyDevice } {}

    VertexStorageLocation VertexStorage::AddVertices(const Vertex1P1N1UV1T1BT* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
    {
        return WriteToUploadBuffers(vertices, vertexCount, indices, indexCount);
    }

    VertexStorageLocation VertexStorage::AddVertices(const Vertex1P1N1UV* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
    {
        return WriteToUploadBuffers(vertices, vertexCount, indices, indexCount);
    }

    VertexStorageLocation VertexStorage::AddVertices(const Vertex1P3* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
    {
        return WriteToUploadBuffers(vertices, vertexCount, indices, indexCount);
    }

    const HAL::BufferResource<Vertex1P1N1UV1T1BT>* VertexStorage::UnifiedVertexBuffer_1P1N1UV1T1BT() const
    {
        return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).VertexBuffer.get();
    }

    const HAL::BufferResource<Vertex1P1N1UV>* VertexStorage::UnifiedVertexBuffer_1P1N1UV() const
    {
        return std::get<FinalBufferPackage<Vertex1P1N1UV>>(mFinalBuffers).VertexBuffer.get();
    }

    const HAL::BufferResource<PathFinder::Vertex1P3>* VertexStorage::UnifiedVertexBuffer_1P() const
    {
        return std::get<FinalBufferPackage<Vertex1P3>>(mFinalBuffers).VertexBuffer.get();
    }

    const HAL::BufferResource<uint32_t>* VertexStorage::UnifiedIndexBuffer_1P1N1UV1T1BT() const
    {
        return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).IndexBuffer.get();
    }

    const HAL::BufferResource<uint32_t>* VertexStorage::UnifiedIndexBuffer_1P1N1UV() const
    {
        return std::get<FinalBufferPackage<Vertex1P1N1UV>>(mFinalBuffers).IndexBuffer.get();
    }

    const HAL::BufferResource<uint32_t>* VertexStorage::UnifiedIndexBuffer_1P() const
    {
        return std::get<FinalBufferPackage<Vertex1P3>>(mFinalBuffers).IndexBuffer.get();
    }

    template <class Vertex>
    VertexStorageLocation PathFinder::VertexStorage::WriteToUploadBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
    {
        auto& package = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);
        auto vertexStartIndex = package.Vertices.size();
        auto indexStartIndex = package.Indices.size();

        auto blasIndex = mBottomAccelerationStructures.size();

        VertexStorageLocation location{ vertexStartIndex, vertexCount, indexStartIndex, indexCount, blasIndex };

        package.Vertices.reserve(package.Vertices.size() + vertexCount);
        package.Indices.reserve(package.Indices.size() + indexCount);
        package.Locations.push_back(location);
        
        std::copy(vertices, vertices + vertexCount, std::back_inserter(package.Vertices));
        std::copy(indices, indices + indexCount, std::back_inserter(package.Indices));

        mBottomAccelerationStructures.emplace_back(mDevice);

        return location;
    }

    template <class Vertex>
    void VertexStorage::CopyBuffersToDefaultHeap()
    {
        auto& uploadBuffers = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);
        auto& finalBuffers = std::get<FinalBufferPackage<Vertex>>(mFinalBuffers);

        if (!uploadBuffers.Vertices.empty())
        {
            auto uploadVertexBuffer = std::make_shared<HAL::BufferResource<Vertex>>(
                *mDevice, uploadBuffers.Vertices.size(), 1, HAL::CPUAccessibleHeapType::Upload);

            uploadVertexBuffer->Write(0, uploadBuffers.Vertices.data(), uploadBuffers.Vertices.size());
            finalBuffers.VertexBuffer = mCopyDevice->QueueResourceCopyToDefaultMemory(uploadVertexBuffer);
            uploadBuffers.Vertices.clear();
        }

        if (!uploadBuffers.Indices.empty())
        {
            auto uploadIndexBuffer = std::make_shared<HAL::BufferResource<uint32_t>>(
                *mDevice, uploadBuffers.Indices.size(), 1, HAL::CPUAccessibleHeapType::Upload);

            uploadIndexBuffer->Write(0, uploadBuffers.Indices.data(), uploadBuffers.Indices.size());
            finalBuffers.IndexBuffer = mCopyDevice->QueueResourceCopyToDefaultMemory(uploadIndexBuffer);
            uploadBuffers.Indices.clear();
        }

        // Allocate RT acceleration structures
        for (const VertexStorageLocation& location : uploadBuffers.Locations)
        {
            HAL::RayTracingBottomAccelerationStructure& blas = mBottomAccelerationStructures[location.BottomAccelerationStructureIndex];

            HAL::RayTracingGeometry<Vertex, uint32_t> blasGeometry{
                finalBuffers.VertexBuffer.get(), location.VertexBufferOffset, location.VertexCount, HAL::ResourceFormat::Color::RGB32_Float,
                finalBuffers.IndexBuffer.get(), location.IndexBufferOffset, location.IndexCount, HAL::ResourceFormat::Color::R32_Unsigned,
                glm::mat4x4{}, true
            };

            blas.AddGeometry(blasGeometry);
            blas.AllocateBuffersIfNeeded();
            blas.SetDebugName("Bottom_Acceleration_Structure");

            mAccelerationStructureBarriers.AddBarrier(HAL::UnorderedAccessResourceBarrier{ blas.FinalBuffer() });
        }
    }

    const HAL::ResourceBarrierCollection& VertexStorage::AccelerationStructureBarriers() const
    {
        return mAccelerationStructureBarriers;
    }

    void VertexStorage::AllocateAndQueueBuffersForCopy()
    {
        CopyBuffersToDefaultHeap<Vertex1P1N1UV1T1BT>();
        CopyBuffersToDefaultHeap<Vertex1P1N1UV>();
        CopyBuffersToDefaultHeap<Vertex1P3>();
    }

}
