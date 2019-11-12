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

        mList->CopyBufferRegion(destination.D3DResource(), destinationOffsetInBytes, source.D3DResource(), sourceOffsetInBytes, regionSizeInBytes);
    }

    template <class T>
    void CopyCommandListBase::CopyBufferToTexture(const BufferResource<T>& buffer, const TextureResource& texture, const SubresourceFootprint& footprint)
    {
        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        D3D12_TEXTURE_COPY_LOCATION dstLocation{};

        srcLocation.pResource = buffer.D3DResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint = footprint.D3DFootprint();

        dstLocation.pResource = texture.D3DResource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = footprint.IndexInResource();

        mList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }

    template <class T>
    void HAL::CopyCommandListBase::CopyTextureToBuffer(const TextureResource& texture, const BufferResource<T>& buffer, const SubresourceFootprint& footprint)
    {
        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        D3D12_TEXTURE_COPY_LOCATION dstLocation{};

        srcLocation.pResource = texture.D3DResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.SubresourceIndex = footprint.IndexInResource();

        dstLocation.pResource = buffer.D3DResource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLocation.PlacedFootprint = footprint.D3DFootprint();

        mList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
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

