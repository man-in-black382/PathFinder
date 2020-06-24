#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <optional>
#include <cstdint>

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
        ResourceTransitionBarrier(
            ResourceState beforeStateMask, 
            ResourceState afterStateMask, 
            const Resource* resource, 
            std::optional<uint64_t> subresourceIndex = std::nullopt);

        ~ResourceTransitionBarrier() = default;

        std::pair<ResourceTransitionBarrier, ResourceTransitionBarrier> Split() const;

    private:
        const Resource* mResource = nullptr;
        ResourceState mBeforeStates = ResourceState::Common;
        ResourceState mAfterStates = ResourceState::Common;

    public:
        inline const auto AssosiatedResource() const { return mResource; }
        inline const auto BeforeStates() const { return mBeforeStates; }
        inline const auto AfterStates() const { return mAfterStates; }
    };



    class ResourceAliasingBarrier : public ResourceBarrier
    {
    public:
        ResourceAliasingBarrier(const Resource* source, const Resource* destination);
        ~ResourceAliasingBarrier() = default;

    private:
        const Resource* mSource;
        const Resource* mDestination;

    public:
        inline const auto SourceResource() const { return mSource; }
        inline const auto DestinationResource() const { return mDestination; }
    };



    class UnorderedAccessResourceBarrier : public ResourceBarrier
    {
    public:
        UnorderedAccessResourceBarrier(const Resource* resource);
        ~UnorderedAccessResourceBarrier() = default;

    private:
        const Resource* mResource;

    public:
        inline const auto UAResource() const { return mResource; }
    };



    class ResourceBarrierCollection
    {
    public:
        void AddBarrier(const ResourceBarrier& barrier);
        void AddBarriers(const ResourceBarrierCollection& barriers);

    private:
        std::vector<D3D12_RESOURCE_BARRIER> mD3DBarriers;

    public:
        inline const D3D12_RESOURCE_BARRIER* D3DBarriers() const { return mD3DBarriers.data(); }
        inline const size_t BarrierCount() const { return mD3DBarriers.size(); }
    };


}

