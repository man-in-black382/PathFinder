#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

#include "DescriptorHeapTypeResolver.hpp"
#include "Resource.hpp"
#include "Descriptor.hpp"
#include "Device.hpp"
#include "Utils.h"

namespace HAL
{
    class DescriptorHeap
    {
    public:
        DescriptorHeap(const Device& device, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE heapType);

    protected:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
        uint32_t mIncrementSize;
        D3D12_CPU_DESCRIPTOR_HANDLE mCurrentHeapHandle;

    public:
        inline const auto D3DPtr() const { return mHeap.Get(); }
    };

    class RTDescriptorHeap : public DescriptorHeap {
    public:
        RTDescriptor EmplaceDescriptorForResource(const Device& device, const ColorTextureResource& resource);
    };

    class DSDescriptorHeap : public DescriptorHeap {
    public:
        DSDescriptor EmplaceDescriptorForResource(const Device& device, const DepthStencilTextureResource& resource);
    };

    class CBSRUADescriptorHeap : public DescriptorHeap {
    public:
        /*CBDescriptor EmplaceDescriptorForConstantBufferResource(const Device& device, const Resource& resource);
        SRDescriptor EmplaceDescriptorForShaderResource(const Device& device, const Resource& resource);
        UADescriptor EmplaceDescriptorForUnorderedAccessResource(const Device& device, const Resource& resource);*/
    };

}

