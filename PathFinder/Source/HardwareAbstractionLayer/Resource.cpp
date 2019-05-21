#include "Resource.hpp"
#include "Utils.h"

#include "../Foundation/Visitor.hpp"

namespace HAL
{
    
    Resource::Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : mResource(existingResourcePtr), mDescription(mResource->GetDesc()) 
    {

    }

    Resource::Resource(const Device& device, const ResourceFormat& format, ResourceState initialStateMask, ResourceState expectedStateMask, HeapType heapType)
        : mDescription(format.D3DResourceDescription())
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        D3D12_RESOURCE_STATES initialStates = D3D12_RESOURCE_STATES(initialStateMask);

        switch (heapType) {
        case HeapType::Default:
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            break;
        case HeapType::Upload:
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            initialStates |= D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        case HeapType::Readback:
            heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
            break;
        }

        SetExpectedUsageFlags(expectedStateMask);

        D3D12_CLEAR_VALUE clearValue;

        ThrowIfFailed(device.D3DPtr()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &mDescription,
            initialStates,
            nullptr,
            IID_PPV_ARGS(&mResource)
        ));
    }

    Resource::~Resource() {}

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
            !EnumMaskBitSet(stateMask, ResourceState::NonPixelShaderAccess))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
    }

}
