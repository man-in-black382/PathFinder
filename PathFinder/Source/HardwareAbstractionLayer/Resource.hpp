#pragma once

#include <wrl.h>
#include <dxgi.h>
#include <cstdint>
#include <optional>
#include <array>
#include <d3d12.h>

#include "Device.hpp"
#include "ResourceFormat.hpp"
#include "ResourceState.hpp"
#include "Utils.h"

#include "GraphicAPIObject.hpp"

#include "../Geometry/Dimensions.hpp"
#include "../Foundation/BitwiseEnum.hpp"

namespace HAL
{

    enum class CPUAccessibleHeapType 
    {
        Upload, Readback 
    };
    
    class Resource : public GraphicAPIObject
    {
    public:
        Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr);

        Resource(const Resource& other) = delete;
        Resource(Resource&& other) = default;

        virtual ~Resource() = 0;

        virtual D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress() const;
        virtual bool CanImplicitlyPromoteFromCommonStateToState(HAL::ResourceState state) const;
        virtual bool CanImplicitlyDecayToCommonStateFromState(HAL::ResourceState state) const;

    protected:
        Resource(
            const Device& device,
            const ResourceFormat& format,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            std::optional<CPUAccessibleHeapType> heapType
        );

        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
        ResourceState mInitialStates;
        ResourceState mExpectedStates;

    private:
        D3D12_RESOURCE_DESC mDescription{};

        void SetExpectedUsageFlags(ResourceState stateMask);

    public:
        inline ID3D12Resource* D3DPtr() const { return mResource.Get(); }
        inline const D3D12_RESOURCE_DESC& D3DDescription() const { return mDescription; };
        inline ResourceState InitialStates() const { return mInitialStates; };
        inline ResourceState ExpectedStates() const { return mExpectedStates; };
    };

}

