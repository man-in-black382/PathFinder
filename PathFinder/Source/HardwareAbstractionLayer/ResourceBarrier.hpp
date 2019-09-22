#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <vector>

#include "Resource.hpp"

namespace HAL
{

    class ResourceBarrier {
    public:
        virtual ~ResourceBarrier() = 0;

    protected:
        D3D12_RESOURCE_BARRIER mDesc{};

    public:
        inline const auto& D3DBarrier() const { return mDesc; }
    };

    class ResourceTransitionBarrier : public ResourceBarrier
    {
    public:
        ResourceTransitionBarrier(ResourceState beforeStateMask, ResourceState afterStateMask, const Resource* resource);
        ~ResourceTransitionBarrier() = default;

    private:
        const Resource* mResource;
        ResourceState mBeforeStates;
        ResourceState mAfterStates;

    public:
        inline const auto AssosiatedResource() const { return mResource; }
        inline const auto BeforeStates() const { return mBeforeStates; }
        inline const auto AfterStates() const { return mAfterStates; }
    };



    class ResourceBarrierCollection
    {
    public:
        void AddBarrier(const ResourceBarrier& barrier);

    private:
        std::vector<D3D12_RESOURCE_BARRIER> mD3DBarriers;

    public:
        inline const D3D12_RESOURCE_BARRIER* D3DBarriers() const { return mD3DBarriers.data(); }
        inline const size_t BarrierCount() const { return mD3DBarriers.size(); }
    };


}

