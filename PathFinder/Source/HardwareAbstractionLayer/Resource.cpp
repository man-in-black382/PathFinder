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

    Resource::Resource(const Device& device, const ResourceFormat& format)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        mDescription = format.D3DResourceDescription();
        mResourceAlignment = format.ResourceAlighnment();
        mTotalMemory = format.ResourceSizeInBytes();

        bool isSubjectForClearing = false;
        D3D12_CLEAR_VALUE d3dClearValue{};

        std::visit(Foundation::MakeVisitor(
            [&](const TextureProperties& props)
            {
                d3dClearValue = D3DClearValue(props.OptimizedClearValue, ResourceFormat::D3DFormat(props.Format));
                mInitialStates = props.InitialStateMask;
                mExpectedStates = props.ExpectedStateMask;

                isSubjectForClearing = 
                    EnumMaskEquals(mExpectedStates, ResourceState::RenderTarget) ||
                    EnumMaskEquals(mExpectedStates, ResourceState::DepthWrite);
            },
            [&](const BufferProperties<uint8_t>& props)
            {
                mInitialStates = props.InitialStateMask;
                mExpectedStates = props.ExpectedStateMask;
            }),
            format.ResourceProperties());

        ThrowIfFailed(device.D3DDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &mDescription,
            D3DResourceState(mInitialStates),
            isSubjectForClearing ? &d3dClearValue : nullptr,
            IID_PPV_ARGS(&mResource)
        ));
    }

    Resource::Resource(const Device& device, const ResourceFormat& format, CPUAccessibleHeapType heapType)
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
        mDescription = format.D3DResourceDescription();
        mResourceAlignment = format.ResourceAlighnment();
        mTotalMemory = format.ResourceSizeInBytes();

        ThrowIfFailed(device.D3DDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &mDescription,
            D3DResourceState(mInitialStates),
            nullptr,
            IID_PPV_ARGS(&mResource)
        ));
    }

    Resource::Resource(const Device& device, const Heap& heap, uint64_t heapOffset, const ResourceFormat& format)
        : mHeapOffset{ heapOffset }
    {
        bool isSubjectForClearing = false;
        D3D12_CLEAR_VALUE d3dClearValue{};

        // Force correct states for Upload/Readback heaps
        if (heap.CPUAccessibleType())
        {
            switch (*heap.CPUAccessibleType())
            {
            case HAL::CPUAccessibleHeapType::Upload: mInitialStates = mExpectedStates = ResourceState::GenericRead; break;
            case HAL::CPUAccessibleHeapType::Readback: mInitialStates = mExpectedStates = ResourceState::CopyDestination; break;
            }
        }
        else
        {
            std::visit(Foundation::MakeVisitor(
                [&](const TextureProperties& props)
                {
                    d3dClearValue = D3DClearValue(props.OptimizedClearValue, ResourceFormat::D3DFormat(props.Format));
                    mInitialStates = props.InitialStateMask;
                    mExpectedStates = props.ExpectedStateMask;

                    isSubjectForClearing =
                        EnumMaskEquals(mExpectedStates, ResourceState::RenderTarget) ||
                        EnumMaskEquals(mExpectedStates, ResourceState::DepthWrite);
                },
                [&](const BufferProperties<uint8_t>& props)
                {
                    mInitialStates = props.InitialStateMask;
                    mExpectedStates = props.ExpectedStateMask;
                }),
                format.ResourceProperties());
        }

        mDescription = format.D3DResourceDescription();
        mResourceAlignment = format.ResourceAlighnment();
        mTotalMemory = format.ResourceSizeInBytes();

        ThrowIfFailed(device.D3DDevice()->CreatePlacedResource(
            heap.D3DHeap(),
            heapOffset,
            &mDescription,
            D3DResourceState(mInitialStates),
            isSubjectForClearing ? &d3dClearValue : nullptr,
            IID_PPV_ARGS(mResource.GetAddressOf())));
    }

    D3D12_CLEAR_VALUE Resource::D3DClearValue(const ClearValue& clearValue, DXGI_FORMAT format) const
    {
        D3D12_CLEAR_VALUE d3dClearValue{};
        d3dClearValue.Format = format;

        std::visit(Foundation::MakeVisitor(
            [&](const ColorClearValue& value)
            {
                d3dClearValue.Color[0] = value[0];
                d3dClearValue.Color[1] = value[1];
                d3dClearValue.Color[2] = value[2];
                d3dClearValue.Color[3] = value[3];
            },
            [&](const DepthStencilClearValue& value)
            {
                d3dClearValue.DepthStencil.Depth = value.Depth;
                d3dClearValue.DepthStencil.Stencil = value.Stencil;
            }),
            clearValue);

        return d3dClearValue;
    }

    Resource::~Resource() {}

    GPUAddress Resource::GPUVirtualAddress() const
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
