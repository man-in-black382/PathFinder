#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "DescriptorHeapTypeResolver.hpp"
#include "Resource.hpp"
#include "Descriptor.hpp"
#include "Device.hpp"

namespace HAL
{
    template <class DescriptorT>
	class DescriptorHeap
	{
    public:
        DescriptorHeap(const Device& device, uint32_t capacity, uint32_t incrementSize);

        void EmplaceDescriptorForResource(const Device& device, const Resource& resource);

    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
        uint32_t mIncrementSize;
        uint32_t mCurrentHeapOffset;

    public:
        inline const auto Heap() const { return mHeap.Get(); }
	};

    template <class DescriptorT>
    DescriptorHeap<DescriptorT>::DescriptorHeap(const Device& device, uint32_t capacity)
        : mIncrementSize(device.Device()->GetDescriptorHandleIncrementSize(DescriptorHeapTypeResolver<DescriptorT>::HeapType()))
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.NumDescriptors = capacity;
        desc.Type = DescriptorHeapTypeResolver<DescriptorT>::HeapType();
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(device.Device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)));
    }

    template <> void DescriptorHeap<RTDescriptor>::EmplaceDescriptorForResource(const Device& device, const Resource& resource)
    {
        
    }

    template <> void DescriptorHeap<DSDescriptor>::EmplaceDescriptorForResource(const Device& device, const Resource& resource)
    {

    }

    template <> void DescriptorHeap<CBDescriptor>::EmplaceDescriptorForResource(const Device& device, const Resource& resource)
    {

    }

    template <> void DescriptorHeap<SRDescriptor>::EmplaceDescriptorForResource(const Device& device, const Resource& resource)
    {

    }

    template <> void DescriptorHeap<UADescriptor>::EmplaceDescriptorForResource(const Device& device, const Resource& resource)
    {

    }

    template <> void DescriptorHeap<SamplerDescriptor>::EmplaceDescriptorForResource(const Device& device, const Resource& resource)
    {

    }

}

