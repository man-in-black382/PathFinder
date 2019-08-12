#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <unordered_map>
#include <tuple>

#include "TextureResource.hpp"
#include "BufferResource.hpp"
#include "Descriptor.hpp"
#include "Device.hpp"
#include "Utils.h"

namespace HAL
{
    template <class... Descriptors>
    class DescriptorHeap
    {
    public:
        struct RangeAllocationInfo
        {
            D3D12_CPU_DESCRIPTOR_HANDLE CurrentCPUHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE CurrentGPUHandle;
            uint32_t InsertedDescriptorCount;
        };

        DescriptorHeap(const Device* device, uint32_t rangeCapacity, uint32_t rangeCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        virtual ~DescriptorHeap() = 0;

    protected:
        template <class T> using DescriptorContainer = std::vector<T>;

        void ValidateCapacity(uint32_t rangeIndex) const;
        void IncrementCounters(uint32_t rangeIndex);
        RangeAllocationInfo& GetRange(uint32_t rangeIndex);

        const Device* mDevice = nullptr;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
        std::tuple<DescriptorContainer<Descriptors>...> mDescriptors;

    private:
        uint32_t mIncrementSize = 0;
        uint32_t mRangeCapacity = 0;

        std::vector<RangeAllocationInfo> mRanges;

    public:
        inline ID3D12DescriptorHeap* D3DHeap() const { return mHeap.Get(); }
    };

    template <class... Descriptors>
    DescriptorHeap<Descriptors...>::DescriptorHeap(const Device* device, uint32_t rangeCapacity, uint32_t rangeCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
        : mDevice{ device }, mRangeCapacity{ rangeCapacity }, mIncrementSize{ device->D3DPtr()->GetDescriptorHandleIncrementSize(heapType) }
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.NumDescriptors = rangeCapacity * rangeCount;
        desc.Type = heapType;
        desc.NodeMask = 0;

        bool shaderVisible = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(device->D3DPtr()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)));

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = mHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle{};

        if (shaderVisible)
        {
            GPUHandle = mHeap->GetGPUDescriptorHandleForHeapStart();
        }

        for (auto rangeIdx = 0u; rangeIdx < rangeCount; rangeIdx++)
        {
            RangeAllocationInfo range{ { CPUHandle.ptr + rangeIdx * mIncrementSize }, { GPUHandle.ptr + rangeIdx * mIncrementSize }, 0 };
            mRanges.push_back(range);
        }
    }

    template <class... Descriptors>
    DescriptorHeap<Descriptors...>::~DescriptorHeap() {}

    template <class... Descriptors>
    void DescriptorHeap<Descriptors...>::ValidateCapacity(uint32_t rangeIndex) const
    {
        if (mRanges[rangeIndex].InsertedDescriptorCount >= mRangeCapacity)
        {
            throw std::runtime_error("Exceeded descriptor heap's capacity");
        }
    }

    template <class... Descriptors>
    void DescriptorHeap<Descriptors...>::IncrementCounters(uint32_t rangeIndex)
    {
        RangeAllocationInfo& range = mRanges[rangeIndex];
        range.CurrentCPUHandle.ptr += mIncrementSize;
        range.CurrentGPUHandle.ptr += mIncrementSize;
        range.InsertedDescriptorCount++;
    }

    template <class... Descriptors>
    typename DescriptorHeap<Descriptors...>::RangeAllocationInfo& DescriptorHeap<Descriptors...>::GetRange(uint32_t rangeIndex)
    {
        return mRanges[rangeIndex];
    }



    class RTDescriptorHeap : public DescriptorHeap<RTDescriptor> {
    public:
        RTDescriptorHeap(const Device* device, uint32_t capacity);
        ~RTDescriptorHeap() = default;

        const RTDescriptor& EmplaceDescriptorForTexture(const ColorTexture& resource);
        const RTDescriptor& EmplaceDescriptorForTexture(const TypelessTexture& resource, ResourceFormat::Color concreteFormat);

    private:
        D3D12_RENDER_TARGET_VIEW_DESC ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc) const;
    };



    class DSDescriptorHeap : public DescriptorHeap<DSDescriptor> {
    public:
        DSDescriptorHeap(const Device* device, uint32_t capacity);
        ~DSDescriptorHeap() = default;

        const DSDescriptor& EmplaceDescriptorForResource(const DepthStencilTexture& resource);

    private:
        D3D12_DEPTH_STENCIL_VIEW_DESC ResourceToDSVDescription(const D3D12_RESOURCE_DESC& resourceDesc) const;
    };



    class CBSRUADescriptorHeap : public DescriptorHeap<CBDescriptor, SRDescriptor, UADescriptor> {        
    public:
        enum class Range : uint8_t
        {
            CBuffer, SBuffer, UABuffer,
            Texture1D, Texture2D, Texture2DArray, Texture3D,
            UATexture1D, UATexture2D, UATexture2DArray, UATexture3D,
            TotalCount
        };

        CBSRUADescriptorHeap(const Device* device, uint32_t rangeCapacity);
        ~CBSRUADescriptorHeap() = default;

        template <class T> const CBDescriptor& EmplaceDescriptorForConstantBuffer(const BufferResource<T>& resource, std::optional<uint64_t> explicitStride = nullopt);
        template <class T> const SRDescriptor& EmplaceDescriptorForStructuredBuffer(const BufferResource<T>& resource);
        template <class T> const UADescriptor& EmplaceDescriptorForUnorderedAccessBuffer(const BufferResource<T>& resource);

        const SRDescriptor& EmplaceDescriptorForTexture(const ColorTexture& texture);
        const SRDescriptor& EmplaceDescriptorForTexture(const DepthStencilTexture& texture);
        const SRDescriptor& EmplaceDescriptorForTexture(const TypelessTexture& texture, ResourceFormat::Color shaderVisibleFormat);

        const UADescriptor& EmplaceDescriptorForUnorderedAccessTexture(const ColorTexture& texture);
        const UADescriptor& EmplaceDescriptorForUnorderedAccessTexture(const DepthStencilTexture& texture);
        const UADescriptor& EmplaceDescriptorForUnorderedAccessTexture(const TypelessTexture& texture, ResourceFormat::Color shaderVisibleFormat);

    private:
        D3D12_SHADER_RESOURCE_VIEW_DESC ResourceToSRVDescription(const D3D12_RESOURCE_DESC& resourceDesc, std::optional<ResourceFormat::Color> explicitFormat = std::nullopt) const;
        D3D12_UNORDERED_ACCESS_VIEW_DESC ResourceToUAVDescription(const D3D12_RESOURCE_DESC& resourceDesc, std::optional<ResourceFormat::Color> explicitFormat = std::nullopt) const;

        const SRDescriptor& EmplaceDescriptorForSRTexture(const TextureResource& texture, std::optional<ResourceFormat::Color> shaderVisibleFormat = std::nullopt);
        const UADescriptor& EmplaceDescriptorForUATexture(const TextureResource& texture, std::optional<ResourceFormat::Color> shaderVisibleFormat = std::nullopt);

        Range RangeTypeForTexture(const TextureResource& texture) const;
        Range UARangeTypeForTexture(const TextureResource& texture) const;
    };

}

#include "DescriptorHeap.inl"

