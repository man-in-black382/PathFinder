#pragma once

#include <wrl.h>
#include <d3d12.h>

namespace HAL
{

    class ResourceBarrier {
    public:
        virtual ~ResourceBarrier() = 0;

    protected:
        D3D12_RESOURCE_BARRIER mDesc;

    public:
        inline const auto& D3DBarrier() const { return mDesc; }
    };

    class ResourceTransitionBarrier : public ResourceBarrier
    {
    public:
        ResourceTransitionBarrier(D3D12_RESOURCE_STATES beforeStates, D3D12_RESOURCE_STATES afterStates, ID3D12Resource* resource);
        ~ResourceTransitionBarrier() = default;
    };

}

