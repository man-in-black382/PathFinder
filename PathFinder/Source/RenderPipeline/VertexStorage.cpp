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
            finalBuffers.VertexBuffer = mCopyDevice->QueueResourceCopyToDefaultHeap(uploadVertexBuffer);
            finalBuffers.VertexBufferDescriptor = std::make_unique<HAL::VertexBufferDescriptor>(*finalBuffers.VertexBuffer);
            uploadBuffers.Vertices.clear();
        }

        if (!uploadBuffers.Indices.empty())
        {
            auto uploadIndexBuffer = std::make_shared<HAL::BufferResource<uint32_t>>(
                *mDevice, uploadBuffers.Indices.size(), 1, HAL::CPUAccessibleHeapType::Upload);

            uploadIndexBuffer->Write(0, uploadBuffers.Indices.data(), uploadBuffers.Indices.size());
            finalBuffers.IndexBuffer = mCopyDevice->QueueResourceCopyToDefaultHeap(uploadIndexBuffer);
            finalBuffers.IndexBufferDescriptor = std::make_unique<HAL::IndexBufferDescriptor>(*finalBuffers.IndexBuffer, HAL::ResourceFormat::Color::R32_Unsigned);
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
            blas.AllocateBuffers();
        }
    }

    const HAL::VertexBufferDescriptor* VertexStorage::UnifiedVertexBufferDescriptorForLayout(VertexLayout layout) const
    {
        switch (layout)
        {
        case VertexLayout::Layout1P1N1UV1T1BT: return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).VertexBufferDescriptor.get();
        case VertexLayout::Layout1P1N1UV: return std::get<FinalBufferPackage<Vertex1P1N1UV>>(mFinalBuffers).VertexBufferDescriptor.get();
        case VertexLayout::Layout1P3: return std::get<FinalBufferPackage<Vertex1P3>>(mFinalBuffers).VertexBufferDescriptor.get();
        default: return nullptr;
        }
    }

    const HAL::IndexBufferDescriptor* VertexStorage::UnifiedIndexBufferDescriptorForLayout(VertexLayout layout) const
    {
        switch (layout)
        {
        case VertexLayout::Layout1P1N1UV1T1BT: return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).IndexBufferDescriptor.get();
        case VertexLayout::Layout1P1N1UV: return std::get<FinalBufferPackage<Vertex1P1N1UV>>(mFinalBuffers).IndexBufferDescriptor.get();
        case VertexLayout::Layout1P3: return std::get<FinalBufferPackage<Vertex1P3>>(mFinalBuffers).IndexBufferDescriptor.get();
        default: return nullptr;
        }
    }

    void VertexStorage::AllocateAndQueueBuffersForCopy()
    {
        CopyBuffersToDefaultHeap<Vertex1P1N1UV1T1BT>();
        CopyBuffersToDefaultHeap<Vertex1P1N1UV>();
        CopyBuffersToDefaultHeap<Vertex1P3>();
    }

}
