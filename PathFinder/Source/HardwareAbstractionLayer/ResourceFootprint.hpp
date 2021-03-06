#pragma once

#include "Resource.hpp"

#include <vector>

namespace HAL
{

    class SubresourceFootprint
    {
    public:
        friend class ResourceFootprint;

        SubresourceFootprint(const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& d3dFootprint, uint32_t rowCount, uint64_t rowSize, uint16_t index);

    private:
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT mD3DFootprint;
        uint32_t mRowCount;
        uint64_t mOffset;
        uint16_t mSubresourceIndex;

        uint64_t mTotalSizeInBytes;

        // Actual number of 'useful' bytes in each row. 
        // It tells us how much data is relevant.
        uint64_t mRowSizeInBytes;

        // The hardware requires a pitch alignment of N bytes, 
        // while each row may have less.
        uint64_t mRowPitch;

        // Row size in bytes and Row pitch may differ in case of block compressed textures.
        // In these cases, the number of rows in the texture is 1/4th the height of the texture 
        // and the RowPitch tells you the number of bytes between two rows of 4x4 blocks.

    public:
        inline const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& D3DFootprint() const { return mD3DFootprint; }
        inline auto RowCount() const { return mRowCount; }
        inline auto RowSizeInBytes() const { return mRowSizeInBytes; }
        inline auto RowPitch() const { return mRowPitch; }
        inline auto Offset() const { return mOffset; }
        inline auto IndexInResource() const { return mSubresourceIndex; }
        inline auto TotalSizeInBytes() const { return mTotalSizeInBytes; }
    };

    class ResourceFootprint
    {
    public:
        ResourceFootprint(const Resource& resource, uint64_t initialByteOffset = 0);

    private:
        std::vector<SubresourceFootprint> mSubresourceFootprints;
        uint64_t mTotalSize = 0;

    public:
        inline const std::vector<SubresourceFootprint>& SubresourceFootprints() const { return mSubresourceFootprints; }
        inline const SubresourceFootprint& GetSubresourceFootprint(uint32_t index) const { return mSubresourceFootprints[index]; }
        inline const auto TotalSizeInBytes() const { return mTotalSize; }
    };

}

