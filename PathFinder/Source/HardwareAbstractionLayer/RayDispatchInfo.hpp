#pragma once

#include <d3d12.h>
#include <cstdint>

namespace HAL
{

    class RayDispatchInfo
    {
    public:
        RayDispatchInfo(const D3D12_DISPATCH_RAYS_DESC& shaderTableAddresses);
        
        void SetWidth(uint64_t width);
        void SetHeight(uint64_t height);
        void SetDepth(uint64_t depth);

    private:
        D3D12_DISPATCH_RAYS_DESC mDesc{};

    public:
        inline const auto& D3DDispatchInfo() const { return mDesc; }
    };

}

