#include "VertexStorage.hpp"

namespace PathFinder
{

    VertexStorage::VertexStorage(HAL::Device* device)
        : mDevice{ device },
        mCommandAllocator{ *device },
        mCommandList{ *device, mCommandAllocator },
        mCommandQueue{ *device }, 
        mFence{ *device } {}


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
    void VertexStorage::AllocateUploadBuffersIfNeeded()
    {
        auto& package = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);

        if (!package.VertexBuffer)
        {
            package.VertexBuffer = std::make_unique<HAL::BufferResource<Vertex>>(
                *mDevice, mUploadBufferCapacity, 1, HAL::ResourceState::Common, HAL::ResourceState::CopySource, HAL::CPUAccessibleHeapType::Upload);
        }

        if (!package.IndexBuffer)
        {
            package.IndexBuffer = std::make_unique<HAL::BufferResource<uint32_t>>(
                *mDevice, mUploadBufferCapacity, 1, HAL::ResourceState::Common, HAL::ResourceState::CopySource, HAL::CPUAccessibleHeapType::Upload);
        }
    }

    template <class Vertex>
    void VertexStorage::AllocateFinalBuffersIfNeeded()
    {
        auto& uploadBuffers = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);
        auto& finalBuffers = std::get<FinalBufferPackage<Vertex>>(mFinalBuffers);

        if (uploadBuffers.VertexBuffer)
        {
            finalBuffers.VertexBuffer = std::make_unique<HAL::BufferResource<Vertex>>(
                *mDevice, uploadBuffers.CurrentVertexOffset, 1, HAL::ResourceState::CopyDestination, HAL::ResourceState::VertexBuffer);
        }
        
        if (uploadBuffers.IndexBuffer)
        {
            finalBuffers.IndexBuffer = std::make_unique<HAL::BufferResource<uint32_t>>(
                *mDevice, uploadBuffers.CurrentIndexOffset, 1, HAL::ResourceState::CopyDestination, HAL::ResourceState::IndexBuffer);
        }
    }

    template <class Vertex>
    VertexStorageLocation PathFinder::VertexStorage::WriteToUploadBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
    {
        AllocateUploadBuffersIfNeeded<Vertex>();

        auto& package = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);

        VertexStorageLocation location{ package.CurrentVertexOffset, vertexCount, package.CurrentIndexOffset, indexCount };

        package.VertexBuffer->Write(location.VertexBufferOffset, vertices, location.VertexCount);
        package.IndexBuffer->Write(location.IndexBufferOffset, indices, location.IndexCount);
        
        package.CurrentVertexOffset += location.VertexCount;
        package.CurrentIndexOffset += location.IndexCount;

        return location;
    }

    template <class Vertex>
    void VertexStorage::CopyBuffersToDefaultHeap()
    {
        AllocateFinalBuffersIfNeeded<Vertex>();

        auto& uploadBuffers = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);
        auto& finalBuffers = std::get<FinalBufferPackage<Vertex>>(mFinalBuffers);

        if (uploadBuffers.VertexBuffer && finalBuffers.VertexBuffer)
        {
            mCommandList.CopyBufferRegion(*uploadBuffers.VertexBuffer, *finalBuffers.VertexBuffer, 0, uploadBuffers.CurrentVertexOffset, 0);
            mCommandList.TransitionResourceState({ HAL::ResourceState::CopyDestination, HAL::ResourceState::VertexBuffer, finalBuffers.VertexBuffer.get() });

            finalBuffers.VertexBufferDescriptor = std::make_unique<HAL::VertexBufferDescriptor>(*finalBuffers.VertexBuffer);
        }
        
        if (uploadBuffers.IndexBuffer && finalBuffers.IndexBuffer)
        {
            mCommandList.CopyBufferRegion(*uploadBuffers.IndexBuffer, *finalBuffers.IndexBuffer, 0, uploadBuffers.CurrentIndexOffset, 0);
            mCommandList.TransitionResourceState({ HAL::ResourceState::CopyDestination, HAL::ResourceState::IndexBuffer, finalBuffers.IndexBuffer.get() });

            finalBuffers.IndexBufferDescriptor = std::make_unique<HAL::IndexBufferDescriptor>(*finalBuffers.IndexBuffer, HAL::ResourceFormat::Color::R32_Unsigned);
        }
        
        mCommandList.Close();
        mCommandQueue.ExecuteCommandList(mCommandList);
        mFence.IncreaseExpectedValue();
        mCommandQueue.SignalFence(mFence);
        mFence.StallCurrentThreadUntilCompletion();
        mCommandAllocator.Reset();
        mCommandList.Reset(mCommandAllocator);

        uploadBuffers.VertexBuffer = nullptr;
        uploadBuffers.IndexBuffer = nullptr;
    }

    const HAL::VertexBufferDescriptor* VertexStorage::UnifiedVertexBufferDescriptorForLayout(VertexLayout layout) const
    {
        switch (layout)
        {
        case VertexLayout::Layout1P1N1UV1T1BT: return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).VertexBufferDescriptor.get();
        case VertexLayout::Layout1P1N1UV: return std::get< FinalBufferPackage<Vertex1P1N1UV>>(mFinalBuffers).VertexBufferDescriptor.get();
        case VertexLayout::Layout1P3: return std::get< FinalBufferPackage<Vertex1P3>>(mFinalBuffers).VertexBufferDescriptor.get();
        }
    }

    const HAL::IndexBufferDescriptor* VertexStorage::UnifiedIndexBufferDescriptorForLayout(VertexLayout layout) const
    {
        switch (layout)
        {
        case VertexLayout::Layout1P1N1UV1T1BT: return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).IndexBufferDescriptor.get();
        case VertexLayout::Layout1P1N1UV: return std::get<FinalBufferPackage<Vertex1P1N1UV>>(mFinalBuffers).IndexBufferDescriptor.get();
        case VertexLayout::Layout1P3: return std::get<FinalBufferPackage<Vertex1P3>>(mFinalBuffers).IndexBufferDescriptor.get();
        }
    }

    void VertexStorage::TransferDataToGPU()
    {
        CopyBuffersToDefaultHeap<Vertex1P1N1UV1T1BT>();
        CopyBuffersToDefaultHeap<Vertex1P1N1UV>();
        CopyBuffersToDefaultHeap<Vertex1P3>();
    }

}
