#include "ResourceFootprint.hpp"

namespace HAL
{
   
    SubresourceFootprint::SubresourceFootprint(const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& d3dFootprint, uint32_t rowCount, uint64_t rowSize)
        : mD3DFootprint{ d3dFootprint }, mRowCount{ rowCount }, mRowSizeInBytes{ rowSize }, mOffset{ d3dFootprint.Offset }, mRowPitch{ d3dFootprint.Footprint.RowPitch } {}

    ResourceFootprint::ResourceFootprint(const Resource& resource)
    {
        auto subresourceCount = resource.SubresourceCount();

        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> d3dFootprints;
        std::vector<uint32_t> rowCounts;
        std::vector<uint64_t> rowSizes;

        d3dFootprints.resize(subresourceCount);
        rowCounts.resize(subresourceCount);
        rowSizes.resize(subresourceCount);

        uint64_t baseOffset = 0;

        Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
        resource.D3DResource()->GetDevice(IID_PPV_ARGS(d3dDevice.GetAddressOf()));
        
        d3dDevice->GetCopyableFootprints(
            &resource.D3DDescription(), 0, subresourceCount, baseOffset,
            &d3dFootprints[0], &rowCounts[0], &rowSizes[0], &mTotalSize);

        for (auto i = 0u; i < subresourceCount; ++i)
        {
            mSubresourceFootprints.emplace_back(d3dFootprints[i], rowCounts[i], rowSizes[i]);
        }
    }

}
