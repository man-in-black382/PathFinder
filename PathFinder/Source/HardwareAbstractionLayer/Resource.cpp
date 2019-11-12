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
        : mInitialStates{ initialStateMask }, mExpectedStates{ expectedStateMask | initialStateMask }, 
        mResourceAlignment{ format.ResourceAlighnment() }, mTotalMemory{ format.ResourceSizeInBytes() }
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        ResourceFormat updatedFormat = format;
        updatedFormat.UpdateExpectedUsageFlags(mExpectedStates);
        mDescription = updatedFormat.D3DResourceDescription();

        bool isSubjectForClearing = 
            EnumMaskBitSet(expectedStateMask, ResourceState::RenderTarget) || 
            EnumMaskBitSet(expectedStateMask, ResourceState::DepthWrite);

        ThrowIfFailed(device.D3DDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &mDescription,
            D3DResourceState(mInitialStates),
            isSubjectForClearing ? format.D3DOptimizedClearValue() : nullptr,
            IID_PPV_ARGS(&mResource)
        ));
    }

    Resource::Resource(const Device& device, const ResourceFormat& format, CPUAccessibleHeapType heapType)
        : mResourceAlignment { format.ResourceAlighnment() }, mTotalMemory{ format.ResourceSizeInBytes() }
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

        ResourceFormat updatedFormat = format;
        updatedFormat.UpdateExpectedUsageFlags(mExpectedStates);
        mDescription = updatedFormat.D3DResourceDescription();

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
        : mInitialStates{ initialStateMask },
        mExpectedStates{ expectedStateMask | initialStateMask },
        mHeapOffset{ heapOffset },
        mResourceAlignment{ format.ResourceAlighnment() },
        mTotalMemory{ format.ResourceSizeInBytes() }
    {
        bool isSubjectForClearing =
            EnumMaskBitSet(expectedStateMask, ResourceState::RenderTarget) ||
            EnumMaskBitSet(expectedStateMask, ResourceState::DepthWrite);

        ResourceFormat updatedFormat = format;
        updatedFormat.UpdateExpectedUsageFlags(mExpectedStates);
        mDescription = updatedFormat.D3DResourceDescription();

        ThrowIfFailed(device.D3DDevice()->CreatePlacedResource(
            heap.D3DHeap(),
            heapOffset,
            &mDescription,
            D3DResourceState(mInitialStates),
            isSubjectForClearing ? format.D3DOptimizedClearValue() : nullptr,
            IID_PPV_ARGS(mResource.GetAddressOf())));
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

}
