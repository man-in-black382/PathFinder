#pragma once

namespace HAL
{
   
    template <class T>
    void CopyCommandListBase::CopyBufferRegion(
        const BufferResource<T>& source, BufferResource<T>& destination,
        uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset)
    {
        if (source.PaddedElementSize() != destination.PaddedElementSize()) {
            throw std::runtime_error("Buffers are misaligned. Copy will lay out data incorrectly.");
        }

        auto sourceOffsetInBytes = source.PaddedElementSize() * sourceOffset;
        auto destinationOffsetInBytes = destination.PaddedElementSize() * destinationOffset;
        auto regionSizeInBytes = source.PaddedElementSize() * objectCount;

        mList->CopyBufferRegion(destination.D3DPtr(), destinationOffsetInBytes, source.D3DPtr(), sourceOffsetInBytes, regionSizeInBytes);
    }



    template <class... Descriptors>
    void ComputeCommandListBase::SetDescriptorHeap(const DescriptorHeap<Descriptors...>& heap)
    {
        auto ptr = heap.D3DHeap();
        mList->SetDescriptorHeaps(1, (ID3D12DescriptorHeap * const*)& ptr);
    }

    template <class T>
    void ComputeCommandListBase::SetComputeRootConstantBuffer(const BufferResource<T>& cbResource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootConstantBufferView(rootParameterIndex, cbResource.GPUVirtualAddress());
    }



    template <class T>
    void GraphicsCommandListBase::SetGraphicsRootConstantBuffer(const BufferResource<T>& cbResource, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootConstantBufferView(rootParameterIndex, cbResource.GPUVirtualAddress());
    }

}

