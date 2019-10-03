#pragma once

#include "Resource.hpp"

#include <vector>

namespace HAL
{

    class SubresourceFootprint
    {
    public:
        SubresourceFootprint(const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& d3dFootprint, uint32_t rowCount, uint64_t rowSize);

    private:
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT mD3DFootprint;
        uint32_t mRowCount;
        uint64_t mRowSizeInBytes;
        uint64_t mOffset;
        uint64_t mRowPitch;

    public:
        inline const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& D3DFootprint() const { return mD3DFootprint; }
        inline auto RowCount() const { return mRowCount; }
        inline auto RowSizeInBytes() const { return mRowSizeInBytes; }
        inline auto RowPitch() const { return mRowPitch; }
        inline auto Offset() const { return mOffset; }
    };

    class ResourceFootprint
    {
    public:
        ResourceFootprint(const Resource& resource);

    private:
        std::vector<SubresourceFootprint> mSubresourceFootprints;
        uint64_t mTotalSize;

    public:
        inline const std::vector<SubresourceFootprint>& SubresourceFootprints() const { return mSubresourceFootprints; }
        inline const SubresourceFootprint& GetSubresourceFootprint(uint32_t index) const { return mSubresourceFootprints[index]; }
        inline const auto TotalSizeInBytes() const { return mTotalSize; }
    };

}

