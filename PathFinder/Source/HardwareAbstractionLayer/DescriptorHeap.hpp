#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <unordered_map>

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
        struct RangeAllocationInfo
        {
            D3D12_CPU_DESCRIPTOR_HANDLE CurrentCPUHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE CurrentGPUHandle;
            uint32_t InsertedDescriptorCount;
        };

        DescriptorHeap(const Device* device, uint32_t rangeCapacity, uint32_t rangeCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        virtual ~DescriptorHeap() = 0;

    protected:
        void ValidateCapacity(uint32_t rangeIndex) const;
        void IncrementCounters(uint32_t rangeIndex);
        RangeAllocationInfo& GetRange(uint32_t rangeIndex);

        const Device* mDevice = nullptr;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;

    private:
        uint32_t mIncrementSize = 0;
        uint32_t mRangeCapacity = 0;

        std::vector<RangeAllocationInfo> mRanges;

    public:
        inline ID3D12DescriptorHeap* D3DHeap() const { return mHeap.Get(); }
    };



    class RTDescriptorHeap : public DescriptorHeap {
    public:
        RTDescriptorHeap(const Device* device, uint32_t capacity);
        ~RTDescriptorHeap() = default;

        RTDescriptor EmplaceDescriptorForTexture(const ColorTextureResource& resource);
        RTDescriptor EmplaceDescriptorForTexture(const TypelessTextureResource& resource, ResourceFormat::Color concreteFormat);

    private:
        D3D12_RENDER_TARGET_VIEW_DESC ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc) const;
    };



    class DSDescriptorHeap : public DescriptorHeap {
    public:
        DSDescriptorHeap(const Device* device, uint32_t capacity);
        ~DSDescriptorHeap() = default;

        DSDescriptor EmplaceDescriptorForResource(const DepthStencilTextureResource& resource);

    private:
        D3D12_DEPTH_STENCIL_VIEW_DESC ResourceToDSVDescription(const D3D12_RESOURCE_DESC& resourceDesc) const;
    };



    class CBSRUADescriptorHeap : public DescriptorHeap {        
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

        template <class T> CBDescriptor EmplaceDescriptorForConstantBuffer(const BufferResource<T>& resource, std::optional<uint64_t> explicitStride = nullopt);
        template <class T> SRDescriptor EmplaceDescriptorForStructuredBuffer(const BufferResource<T>& resource);
        template <class T> UADescriptor EmplaceDescriptorForUnorderedAccessBuffer(const BufferResource<T>& resource);

        SRDescriptor EmplaceDescriptorForTexture(const ColorTextureResource& texture);
        SRDescriptor EmplaceDescriptorForTexture(const DepthStencilTextureResource& texture);
        SRDescriptor EmplaceDescriptorForTexture(const TypelessTextureResource& texture, ResourceFormat::Color shaderVisibleFormat);

        UADescriptor EmplaceDescriptorForUnorderedAccessTexture(const ColorTextureResource& texture);
        UADescriptor EmplaceDescriptorForUnorderedAccessTexture(const DepthStencilTextureResource& texture);
        UADescriptor EmplaceDescriptorForUnorderedAccessTexture(const TypelessTextureResource& texture, ResourceFormat::Color shaderVisibleFormat);

    private:
        D3D12_SHADER_RESOURCE_VIEW_DESC ResourceToSRVDescription(const D3D12_RESOURCE_DESC& resourceDesc, std::optional<ResourceFormat::Color> explicitFormat = std::nullopt) const;
        D3D12_UNORDERED_ACCESS_VIEW_DESC ResourceToUAVDescription(const D3D12_RESOURCE_DESC& resourceDesc, std::optional<ResourceFormat::Color> explicitFormat = std::nullopt) const;

        SRDescriptor EmplaceDescriptorForSRTexture(const TextureResource& texture, std::optional<ResourceFormat::Color> shaderVisibleFormat = std::nullopt);
        UADescriptor EmplaceDescriptorForUATexture(const TextureResource& texture, std::optional<ResourceFormat::Color> shaderVisibleFormat = std::nullopt);

        Range RangeTypeForTexture(const TextureResource& texture) const;
        Range UARangeTypeForTexture(const TextureResource& texture) const;
    };

}

#include "DescriptorHeap.inl"

