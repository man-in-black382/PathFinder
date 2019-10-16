#include "Resource.hpp"
#include "Utils.h"
#include "ResourceFootprint.hpp"

#include "../Foundation/STDHelpers.hpp"
#include "../Foundation/StringUtils.hpp"

#include <vector>

namespace HAL
{

    Resource::Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : mResource(existingResourcePtr), mDescription(mResource->GetDesc()) {}

    Resource::Resource(const Device& device, const ResourceFormat& format, ResourceState initialStateMask, ResourceState expectedStateMask)
        : mDescription{ format.D3DResourceDescription() }, 
        mInitialStates{ initialStateMask }, mExpectedStates{ expectedStateMask }, 
        mResourceAlignment{ format.ResourceAlighnment() }, mTotalMemory{ format.ResourceSizeInBytes() }
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_STATES initialStates = D3DResourceState(initialStateMask);

        SetExpectedUsageFlags(expectedStateMask);

        bool isSubjectForClearing = 
            EnumMaskBitSet(expectedStateMask, ResourceState::RenderTarget) || 
            EnumMaskBitSet(expectedStateMask, ResourceState::DepthWrite);

        ThrowIfFailed(device.D3DDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &mDescription,
            initialStates,
            isSubjectForClearing ? format.D3DOptimizedClearValue() : nullptr,
            IID_PPV_ARGS(&mResource)
        ));
    }

    Resource::Resource(const Device& device, const ResourceFormat& format, CPUAccessibleHeapType heapType)
        : mDescription{ format.D3DResourceDescription() },
        mResourceAlignment { format.ResourceAlighnment() },
        mTotalMemory{ format.ResourceSizeInBytes() }
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        ResourceState initialState = ResourceState::Common;

        switch (heapType)
        {
        case CPUAccessibleHeapType::Upload:
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            mInitialStates = ResourceState::GenericRead;
            break;

        case CPUAccessibleHeapType::Readback:
            heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
            mInitialStates = ResourceState::CopyDestination;
            break;
        }

        mExpectedStates = mInitialStates;

        SetExpectedUsageFlags(mInitialStates);

        ThrowIfFailed(device.D3DDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &mDescription,
            D3DResourceState(mInitialStates),
            nullptr,
            IID_PPV_ARGS(&mResource)
        ));
    }

    Resource::Resource(const Device& device, const Heap& heap, uint64_t heapOffset, const ResourceFormat& format, ResourceState initialStateMask, ResourceState expectedStateMask)
        : mDescription{ format.D3DResourceDescription() }, mInitialStates{ initialStateMask },
        mExpectedStates{ expectedStateMask }, mHeapOffset{ heapOffset },
        mResourceAlignment{ format.ResourceAlighnment() }, mTotalMemory{ format.ResourceSizeInBytes() }
    {
        SetExpectedUsageFlags(expectedStateMask);

        bool isSubjectForClearing =
            EnumMaskBitSet(expectedStateMask, ResourceState::RenderTarget) ||
            EnumMaskBitSet(expectedStateMask, ResourceState::DepthWrite);

        device.D3DDevice()->CreatePlacedResource(
            heap.D3DHeap(),
            heapOffset,
            &mDescription,
            D3DResourceState(initialStateMask),
            isSubjectForClearing ? format.D3DOptimizedClearValue() : nullptr,
            IID_PPV_ARGS(mResource.GetAddressOf()));
    }

    Resource::~Resource() {}

    D3D12_GPU_VIRTUAL_ADDRESS Resource::GPUVirtualAddress() const
    {
        return mResource->GetGPUVirtualAddress();
    }

    uint32_t Resource::SubresourceCount() const
    {
        return 1;
    }

    void Resource::SetDebugName(const std::string& name)
    {
        mResource->SetName(StringToWString(name).c_str());
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
            !EnumMaskBitSet(stateMask, ResourceState::ConstantBuffer) &&
            !EnumMaskBitSet(stateMask, ResourceState::UnorderedAccess))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
    }

}
