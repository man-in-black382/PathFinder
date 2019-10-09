#include "Resource.hpp"
#include "Utils.h"
#include "ResourceFootprint.hpp"

#include "../Foundation/STDHelpers.hpp"

#include <vector>

namespace HAL
{

    Resource::Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : mResource(existingResourcePtr), mDescription(mResource->GetDesc()) {}

    Resource::Resource(const Device& device, const ResourceFormat& format, ResourceState initialStateMask, ResourceState expectedStateMask)
        : mDescription{ format.D3DResourceDescription() }, mInitialStates{ initialStateMask }, mExpectedStates{ expectedStateMask }
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

        QueryAllocationInfo(device);
    }

    Resource::Resource(const Device& device, const ResourceFormat& format, CPUAccessibleHeapType heapType)
        : mDescription{ format.D3DResourceDescription() }
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

        QueryAllocationInfo(device);
    }

    Resource::Resource(const Device& device, const Heap& heap, uint64_t heapOffset, const ResourceFormat& format, ResourceState initialStateMask, ResourceState expectedStateMask)
        : mDescription{ format.D3DResourceDescription() }, mInitialStates{ initialStateMask }, mExpectedStates{ expectedStateMask }, mHeapOffset{ heapOffset }
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

        QueryAllocationInfo(device);
    }

    Resource::~Resource() {}

    D3D12_GPU_VIRTUAL_ADDRESS Resource::GPUVirtualAddress() const
    {
        return mResource->GetGPUVirtualAddress();
    }

    bool Resource::CanImplicitlyPromoteFromCommonStateToState(ResourceState state) const
    {
        return false;
    }

    bool Resource::CanImplicitlyDecayToCommonStateFromState(ResourceState state) const
    {
        return false;
    }

    uint32_t Resource::SubresourceCount() const
    {
        return 1;
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

    void Resource::QueryAllocationInfo(const Device& device)
    {
        UINT GPUMask = 0;
        D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device.D3DDevice()->GetResourceAllocationInfo(GPUMask, 1, &mDescription);

        mResourceAlignment = allocInfo.Alignment;
        mTotalMemory = allocInfo.SizeInBytes;
    }

}
