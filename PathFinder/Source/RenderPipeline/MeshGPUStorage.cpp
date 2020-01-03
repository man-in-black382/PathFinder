#include "MeshGPUStorage.hpp"

#include <algorithm>
#include <iterator>

namespace PathFinder
{

    MeshGPUStorage::MeshGPUStorage(HAL::Device* device, CopyDevice* copyDevice, uint8_t simultaneousFramesInFlight)
        : mDevice{ device },
        mCopyDevice{ copyDevice },
        mInstanceTable{ *device, 1024, simultaneousFramesInFlight, 256, HAL::CPUAccessibleHeapType::Upload },
        mTopAccelerationStructure{ device } {}

    void MeshGPUStorage::StoreMesh(Mesh& mesh)
    {
        VertexStorageLocation locationInStorage = WriteToUploadBuffers(
            mesh.Vertices().data(), mesh.Vertices().size(), mesh.Indices().data(), mesh.Indices().size());

        mesh.SetVertexStorageLocation(locationInStorage);
    }

    const HAL::BufferResource<Vertex1P1N1UV1T1BT>* MeshGPUStorage::UnifiedVertexBuffer_1P1N1UV1T1BT() const
    {
        return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).VertexBuffer.get();
    }

    const HAL::BufferResource<Vertex1P1N1UV>* MeshGPUStorage::UnifiedVertexBuffer_1P1N1UV() const
    {
        return std::get<FinalBufferPackage<Vertex1P1N1UV>>(mFinalBuffers).VertexBuffer.get();
    }

    const HAL::BufferResource<PathFinder::Vertex1P3>* MeshGPUStorage::UnifiedVertexBuffer_1P() const
    {
        return std::get<FinalBufferPackage<Vertex1P3>>(mFinalBuffers).VertexBuffer.get();
    }

    const HAL::BufferResource<uint32_t>* MeshGPUStorage::UnifiedIndexBuffer_1P1N1UV1T1BT() const
    {
        return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).IndexBuffer.get();
    }

    const HAL::BufferResource<uint32_t>* MeshGPUStorage::UnifiedIndexBuffer_1P1N1UV() const
    {
        return std::get<FinalBufferPackage<Vertex1P1N1UV>>(mFinalBuffers).IndexBuffer.get();
    }

    const HAL::BufferResource<uint32_t>* MeshGPUStorage::UnifiedIndexBuffer_1P() const
    {
        return std::get<FinalBufferPackage<Vertex1P3>>(mFinalBuffers).IndexBuffer.get();
    }

    template <class Vertex>
    VertexStorageLocation MeshGPUStorage::WriteToUploadBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
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
    void MeshGPUStorage::CopyBuffersToDefaultHeap()
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
    }

    template <class Vertex>
    void MeshGPUStorage::CreateBottomAccelerationStructuresInternal()
    {
        // BLAS is created when mesh vertices are added. Here we only setup geometry inputs and allocate upload buffers.

        auto& uploadBuffers = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);
        auto& finalBuffers = std::get<FinalBufferPackage<Vertex>>(mFinalBuffers);

        mBottomASBuildBarriers = HAL::ResourceBarrierCollection{};

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
            blas.SetDebugName("Bottom Acceleration Structure");

            mBottomASBuildBarriers.AddBarrier(HAL::UnorderedAccessResourceBarrier{ blas.FinalBuffer() });
        }

        uploadBuffers.Locations.clear();
    }

    void MeshGPUStorage::BeginFrame(uint64_t newFrameNumber)
    {
        mCurrentFrameInsertedInstanceCount = 0;
        mTopAccelerationStructure.ResetInputs();
        mInstanceTable.PrepareMemoryForNewFrame(newFrameNumber);
    }

    void MeshGPUStorage::EndFrame(uint64_t completedFrameNumber)
    {
        mInstanceTable.DiscardMemoryForCompletedFrames(completedFrameNumber);
    }

    void MeshGPUStorage::UploadVerticesAndQueueForCopy()
    {
        CopyBuffersToDefaultHeap<Vertex1P1N1UV1T1BT>();
        CopyBuffersToDefaultHeap<Vertex1P1N1UV>();
        CopyBuffersToDefaultHeap<Vertex1P3>();
    }

    void MeshGPUStorage::CreateBottomAccelerationStructures()
    {
        CreateBottomAccelerationStructuresInternal<Vertex1P1N1UV1T1BT>();
        CreateBottomAccelerationStructuresInternal<Vertex1P1N1UV>();
        CreateBottomAccelerationStructuresInternal<Vertex1P3>();
    }

    void MeshGPUStorage::CreateTopAccelerationStructure()
    {
        mTopAccelerationStructure.AllocateBuffersIfNeeded();
        mTopAccelerationStructure.SetDebugName("Unified Top RT Acceleration Structure");
        mTopASBuildBarriers = HAL::ResourceBarrierCollection{};
        mTopASBuildBarriers.AddBarrier(HAL::UnorderedAccessResourceBarrier{ mTopAccelerationStructure.FinalBuffer() });
    }

    void MeshGPUStorage::StoreMeshInstance(MeshInstance& instance, const HAL::RayTracingBottomAccelerationStructure& blas)
    {
        assert_format(mCurrentFrameInsertedInstanceCount < mInstanceTable.PerFrameCapacity(),
            "Buffer capacity is static in current implementation and you have reached a maximum amount of updates per frame");

        GPUInstanceTableEntry instanceEntry{
            instance.Transformation().ModelMatrix(),
            instance.Transformation().NormalMatrix(),
            instance.AssosiatedMaterial()->AlbedoMapSRVIndex,
            instance.AssosiatedMaterial()->NormalMapSRVIndex,
            instance.AssosiatedMaterial()->RoughnessMapSRVIndex,
            instance.AssosiatedMaterial()->MetalnessMapSRVIndex,
            instance.AssosiatedMaterial()->AOMapSRVIndex,
            instance.AssosiatedMaterial()->DisplacementMapSRVIndex,
            instance.AssosiatedMaterial()->DistanceAtlasIndirectionMapSRVIndex,
            instance.AssosiatedMaterial()->DistanceAtlasSRVIndex,
            instance.AssosiatedMesh()->LocationInVertexStorage().VertexBufferOffset,
            instance.AssosiatedMesh()->LocationInVertexStorage().IndexBufferOffset,
            instance.AssosiatedMesh()->LocationInVertexStorage().IndexCount
        };

        GPUInstanceIndex instanceIndex = mCurrentFrameInsertedInstanceCount;

        mInstanceTable.Write(mCurrentFrameInsertedInstanceCount, &instanceEntry);
        ++mCurrentFrameInsertedInstanceCount;

        mTopAccelerationStructure.AddInstance(blas, instanceIndex, instance.Transformation().ModelMatrix());
        instance.SetGPUInstanceIndex(instanceIndex);
    }

}
