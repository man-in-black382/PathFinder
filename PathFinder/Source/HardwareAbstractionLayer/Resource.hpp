#pragma once

#include <wrl.h>
#include <dxgi.h>
#include <cstdint>
#include <optional>
#include <d3d12.h>

#include "Device.hpp"
#include "ResourceFormat.hpp"
#include "ResourceState.hpp"
#include "Utils.h"

#include "../Geometry/Dimensions.hpp"
#include "../Foundation/BitwiseEnum.hpp"

namespace HAL
{

    enum class CPUAccessibleHeapType 
    {
        Upload, Readback 
    };
    
    class Resource
    {
    public:
        Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr);

        Resource(const Resource& other) = delete;
        Resource(Resource&& other) = default;

        virtual ~Resource() = 0;

        virtual D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress() const;

    protected:
        Resource(
            const Device& device,
            const ResourceFormat& format,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            std::optional<CPUAccessibleHeapType> heapType
        );

        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;

    private:
        D3D12_RESOURCE_DESC mDescription{};

        void SetExpectedUsageFlags(ResourceState stateMask);

    public:
        inline const auto D3DPtr() const { return mResource.Get(); }
        inline const auto& D3DDescription() const { return mDescription; };
    };

}

