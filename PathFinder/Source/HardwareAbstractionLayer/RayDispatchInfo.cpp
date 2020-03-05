#include "RayDispatchInfo.hpp"

namespace HAL
{

    RayDispatchInfo::RayDispatchInfo(const D3D12_DISPATCH_RAYS_DESC& shaderTableAddresses)
        : mDesc{ shaderTableAddresses } {}

    void RayDispatchInfo::SetWidth(uint64_t width)
    {
        mDesc.Width = width;
    }

    void RayDispatchInfo::SetHeight(uint64_t height)
    {
        mDesc.Height = height;
    }

    void RayDispatchInfo::SetDepth(uint64_t depth)
    {
        mDesc.Depth = depth;
    }

}
