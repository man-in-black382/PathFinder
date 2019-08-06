#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

#include "TextureResource.hpp"
#include "BufferResource.hpp"
#include "Descriptor.hpp"
#include "Device.hpp"
#include "Utils.h"

namespace HAL
{
    class DescriptorHeap
    {
    public:
        DescriptorHeap(const Device* device, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        virtual ~DescriptorHeap() = 0;

    protected:
        void ValidateCapacity() const;
        void IncrementCounters();

        const Device* mDevice = nullptr;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
        uint32_t mIncrementSize = 0;
        uint32_t mCapacity = 0;
        uint32_t mInsertedDescriptorCount = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE mCurrentHeapHandle;

    public:
        inline const auto D3DPtr() const { return mHeap.Get(); }
    };


    class RTDescriptorHeap : public DescriptorHeap {
    public:
        RTDescriptorHeap(const Device* device, uint32_t capacity);
        ~RTDescriptorHeap() = default;

        RTDescriptor EmplaceDescriptorForResource(const ColorTextureResource& resource);
        RTDescriptor EmplaceDescriptorForResource(const TypelessTextureResource& resource, ResourceFormat::Color concreteFormat);
    };


    class DSDescriptorHeap : public DescriptorHeap {
    public:
        DSDescriptorHeap(const Device* device, uint32_t capacity);
        ~DSDescriptorHeap() = default;

        DSDescriptor EmplaceDescriptorForResource(const DepthStencilTextureResource& resource);
    };

    class CBSRUADescriptorHeap : public DescriptorHeap {
    public:
        CBSRUADescriptorHeap(const Device* device, uint32_t capacity);
        ~CBSRUADescriptorHeap() = default;

        template <class T> CBDescriptor EmplaceDescriptorForConstantBufferResource(const Device& device, const BufferResource<T>& resource);
        template <class T> CBDescriptor EmplaceDescriptorForConstantBufferResource(const Device& device, const BufferResource<T>& resource, uint64_t byteCount);
     /*   SRDescriptor EmplaceDescriptorForShaderResource(const Device& device, const Resource& resource);
        UADescriptor EmplaceDescriptorForUnorderedAccessResource(const Device& device, const Resource& resource);*/
    };

}

#include "DescriptorHeap.inl"

