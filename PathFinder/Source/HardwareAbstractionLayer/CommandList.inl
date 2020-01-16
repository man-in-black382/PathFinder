#pragma once

#include "../Foundation/Assert.hpp"

namespace HAL
{
   
    template <class T>
    void CopyCommandListBase::CopyBufferRegion(
        const Buffer<T>& source, const Buffer<T>& destination,
        uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset)
    {
        assert_format(source.PaddedElementSize() == destination.PaddedElementSize(), "Buffers are misaligned. Copy will lay out data incorrectly.");

        auto sourceOffsetInBytes = source.PaddedElementSize() * sourceOffset;
        auto destinationOffsetInBytes = destination.PaddedElementSize() * destinationOffset;
        auto regionSizeInBytes = source.PaddedElementSize() * objectCount;

        mList->CopyBufferRegion(destination.D3DResource(), destinationOffsetInBytes, source.D3DResource(), sourceOffsetInBytes, regionSizeInBytes);
    }

    template <class T>
    void CopyCommandListBase::CopyBufferToTexture(const Buffer<T>& buffer, const Texture& texture, const SubresourceFootprint& footprint)
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
    void HAL::CopyCommandListBase::CopyTextureToBuffer(const Texture& texture, const Buffer<T>& buffer, const SubresourceFootprint& footprint)
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



    template <class T>
    void ComputeCommandListBase::SetComputeRootConstantBuffer(const Buffer<T>& cbResource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootConstantBufferView(rootParameterIndex, cbResource.GPUVirtualAddress());
    }

    template <class T>
    void ComputeCommandListBase::SetComputeRootConstants(const T& constants, uint32_t rootParameterIndex)
    {
        mList->SetComputeRoot32BitConstants(rootParameterIndex, sizeof(T) / 4, &constants, 0);
    }



    template <class T>
    void GraphicsCommandListBase::SetGraphicsRootConstantBuffer(const Buffer<T>& cbResource, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootConstantBufferView(rootParameterIndex, cbResource.GPUVirtualAddress());
    }

    template <class T>
    void GraphicsCommandListBase::SetGraphicsRootConstants(const T& constants, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRoot32BitConstants(rootParameterIndex, sizeof(T) / 4, &constants, 0);
    }

}

