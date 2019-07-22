#include "MeshGPUStorage.hpp"

namespace PathFinder
{

    MeshGPUStorage::MeshGPUStorage(HAL::Device* device)
        : mDevice{ device },
        mCommandAllocator{ *device },
        mCommandList{ *device, mCommandAllocator },
        mCommandQueue{ *device }, 
        mFence{ *device } {}

    MeshInstance MeshGPUStorage::EmplaceInstanceForMesh(const Mesh* mesh)
    {
        if (!mUploadVertexBuffer)
        {
            mUploadVertexBuffer = std::make_unique<HAL::BufferResource<Vertex1P1N1UV1T1BT>>(
                *mDevice, mUploadBufferCapacity, 1, HAL::ResourceState::Common, HAL::ResourceState::CopySource, HAL::HeapType::Upload);
        }

        if (!mUploadIndexBuffer)
        {
            mUploadIndexBuffer = std::make_unique<HAL::BufferResource<uint32_t>>(
                *mDevice, mUploadBufferCapacity, 1, HAL::ResourceState::Common, HAL::ResourceState::CopySource, HAL::HeapType::Upload);
        }

        auto bufferLocationsIt = mEmplacedMeshBufferLocations.find(mesh);
        bool meshAlreadyEmplaced = bufferLocationsIt != mEmplacedMeshBufferLocations.end();

        if (meshAlreadyEmplaced)
        {
            return MeshInstance{ mesh, bufferLocationsIt->second };
        }
        
        std::vector<MeshInstance::GPUBufferLocation> gpuBufferLocations;

        for (const SubMesh& subMesh : mesh->SubMeshes())
        {
            MeshInstance::GPUBufferLocation gpuLocation{ mCurrentVertexOffset, subMesh.Vertices().size(), mCurrentIndexOffset, subMesh.Indices().size() };
            mUploadVertexBuffer->Write(gpuLocation.vertexBufferOffset, subMesh.Vertices().data(), gpuLocation.vertexCount);
            mUploadIndexBuffer->Write(gpuLocation.indexBufferOffset, subMesh.Indices().data(), gpuLocation.indexCount);
            mCurrentVertexOffset += gpuLocation.vertexCount;
            mCurrentIndexOffset += gpuLocation.indexCount;
            gpuBufferLocations.push_back(gpuLocation);
        }

        mEmplacedMeshBufferLocations[mesh] = gpuBufferLocations;

        return MeshInstance{ mesh, gpuBufferLocations };
    }

    void MeshGPUStorage::TransferDataToGPU()
    {
        mFinalVertexBuffer = std::make_unique<HAL::BufferResource<Vertex1P1N1UV1T1BT>>(
            *mDevice, mCurrentVertexOffset, 1, HAL::ResourceState::CopyDestination, HAL::ResourceState::VertexBuffer
        );

        mFinalIndexBuffer = std::make_unique<HAL::BufferResource<uint32_t>>(
           *mDevice, mCurrentIndexOffset, 1, HAL::ResourceState::CopyDestination, HAL::ResourceState::IndexBuffer
        );

        mCommandList.CopyBufferRegion(*mUploadVertexBuffer, *mFinalVertexBuffer, 0, mCurrentVertexOffset, 0);
        mCommandList.TransitionResourceState({ HAL::ResourceState::CopyDestination, HAL::ResourceState::VertexBuffer, mFinalVertexBuffer.get() });

        mCommandList.CopyBufferRegion(*mUploadIndexBuffer, *mFinalIndexBuffer, 0, mCurrentIndexOffset, 0);
        mCommandList.TransitionResourceState({ HAL::ResourceState::CopyDestination, HAL::ResourceState::IndexBuffer, mFinalIndexBuffer.get() });

        mCommandList.Close();
        mCommandQueue.ExecuteCommandList(mCommandList);
        mCommandQueue.StallCPUUntilDone(mFence);
        mCommandAllocator.Reset(); 
        mCommandList.Reset(mCommandAllocator);

        mUploadVertexBuffer = nullptr;
        mUploadIndexBuffer = nullptr;

        mVertexBufferDescriptor = std::make_unique<HAL::VertexBufferDescriptor>(*mFinalVertexBuffer);
        mIndexBufferDescriptor = std::make_unique<HAL::IndexBufferDescriptor>(*mFinalIndexBuffer, HAL::ResourceFormat::Color::R32_Unsigned);
    }

}
