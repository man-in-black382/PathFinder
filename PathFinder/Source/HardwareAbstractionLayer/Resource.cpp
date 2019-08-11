#include "Resource.hpp"
#include "Utils.h"

#include "../Foundation/STDHelpers.hpp"

namespace HAL
{

    Resource::Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : mResource(existingResourcePtr), mDescription(mResource->GetDesc()) {}

    Resource::Resource(
        const Device& device,
        const ResourceFormat& format, 
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        std::optional<ClearValue> optimizedClearValue,
        std::optional<CPUAccessibleHeapType> heapType)
        :
        mDescription(format.D3DResourceDescription())
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        D3D12_RESOURCE_STATES initialStates = D3DResourceState(initialStateMask);

        if (heapType)
        {
            switch (*heapType)
            {
            case CPUAccessibleHeapType::Upload:
                heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
                initialStates |= D3D12_RESOURCE_STATE_GENERIC_READ;
                break;

            case CPUAccessibleHeapType::Readback:
                heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
                break;
            }
        }
        else {
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        }

        SetExpectedUsageFlags(expectedStateMask);

        D3D12_CLEAR_VALUE d3dClearValue{};
        d3dClearValue.Format = format.D3DResourceDescription().Format;

        if (optimizedClearValue)
        {
            std::visit(Foundation::MakeVisitor(
                [&d3dClearValue](const ColorClearValue& value)
            {
                d3dClearValue.Color[0] = value[0];
                d3dClearValue.Color[1] = value[1];
                d3dClearValue.Color[2] = value[2];
                d3dClearValue.Color[3] = value[3];
            },
                [&d3dClearValue](const DepthStencilClearValue& value)
            {
                d3dClearValue.DepthStencil.Depth = value.Depth;
                d3dClearValue.DepthStencil.Stencil = value.Stencil;
            }),
                *optimizedClearValue);
        }

        ThrowIfFailed(device.D3DPtr()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &mDescription,
            initialStates,
            optimizedClearValue ? &d3dClearValue : nullptr,
            IID_PPV_ARGS(&mResource)
        ));
    }

    Resource::~Resource() {}

    D3D12_GPU_VIRTUAL_ADDRESS Resource::GPUVirtualAddress() const
    {
        return mResource->GetGPUVirtualAddress();
    }

    void Resource::SetExpectedUsageFlags(ResourceState stateMask)
    {
        if (EnumMaskBitSet(stateMask, ResourceState::RenderTarget))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }

        if (EnumMaskBitSet(stateMask, ResourceState::DepthRead) ||
            EnumMaskBitSet(stateMask, ResourceState::DepthWrite))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }

        if (EnumMaskBitSet(stateMask, ResourceState::UnorderedAccess))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        if (!EnumMaskBitSet(stateMask, ResourceState::PixelShaderAccess) &&
            !EnumMaskBitSet(stateMask, ResourceState::NonPixelShaderAccess) &&
            !EnumMaskBitSet(stateMask, ResourceState::RenderTarget) &&
            !EnumMaskBitSet(stateMask, ResourceState::GenericRead) &&
            !EnumMaskBitSet(stateMask, ResourceState::IndirectArgument) &&
            !EnumMaskBitSet(stateMask, ResourceState::CopySource) &&
            !EnumMaskBitSet(stateMask, ResourceState::DepthRead) &&
            !EnumMaskBitSet(stateMask, ResourceState::ConstantBuffer))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
    }

}
