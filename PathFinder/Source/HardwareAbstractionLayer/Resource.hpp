#pragma once

#include <wrl.h>
#include <dxgi.h>
#include <cstdint>
#include <optional>
#include <array>
#include <functional>
#include <d3d12.h>

#include "Device.hpp"
#include "Heap.hpp"
#include "ResourceFormat.hpp"
#include "ResourceState.hpp"
#include "Utils.h"
#include "Types.hpp"

#include "GraphicAPIObject.hpp"

#include "../Geometry/Dimensions.hpp"
#include "../Foundation/BitwiseEnum.hpp"

namespace HAL
{
    
    class Resource : public GraphicAPIObject
    {
    public:
        using DeallocationCallback = std::function<void()>;

        Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr);
        Resource(const Resource& other) = delete;
        Resource(Resource&& other) = default;

        virtual ~Resource() = 0;

        virtual GPUAddress GPUVirtualAddress() const;
        virtual uint32_t SubresourceCount() const;

        virtual bool CanImplicitlyPromoteFromCommonStateToState(ResourceState state) const = 0;
        virtual bool CanImplicitlyDecayToCommonStateFromState(ResourceState state) const = 0;

        virtual void SetDebugName(const std::string& name) override;

    protected:
        Resource(const Device& device, const ResourceFormat& format);
        Resource(const Device& device, const ResourceFormat& format, CPUAccessibleHeapType heapType);
        Resource(const Device& device, const Heap& heap, uint64_t heapOffset, const ResourceFormat& format);

        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
        ResourceState mInitialStates = ResourceState::Common;
        ResourceState mExpectedStates = ResourceState::Common;

    private:
        D3D12_CLEAR_VALUE D3DClearValue(const ClearValue& clearValue, DXGI_FORMAT format) const;

        uint64_t mTotalMemory = 0;
        uint64_t mResourceAlignment = 0;
        uint64_t mSubresourceCount = 0;
        uint64_t mHeapOffset = 0;
        D3D12_RESOURCE_DESC mDescription{};

    public:
        inline ID3D12Resource* D3DResource() const { return mResource.Get(); }
        inline const D3D12_RESOURCE_DESC& D3DDescription() const { return mDescription; };
        inline auto InitialStates() const { return mInitialStates; };
        inline auto ExpectedStates() const { return mExpectedStates; };
        inline auto TotalMemory() const { return mTotalMemory; }
        inline auto ResourceAlignment() const { return mResourceAlignment; }
        inline auto HeapOffset() const { return mHeapOffset; }
    };

}

