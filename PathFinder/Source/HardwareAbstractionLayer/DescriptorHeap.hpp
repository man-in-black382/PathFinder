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

        uint32_t Capacity() const;

    protected:
        template <class T> using DescriptorContainer = std::vector<T>;

        void ValidateCapacity(uint32_t rangeIndex) const;
        void IncrementCounters(uint32_t rangeIndex);
        RangeAllocationInfo& GetRange(uint32_t rangeIndex);

        const Device* mDevice = nullptr;
        uint32_t mRangeCapacity = 0;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
        std::tuple<DescriptorContainer<Descriptors>...> mDescriptors;

    private:
        uint32_t mIncrementSize = 0;

        std::vector<RangeAllocationInfo> mRanges;

    public:
        inline ID3D12DescriptorHeap* D3DHeap() const { return mHeap.Get(); }
    };



    class RTDescriptorHeap : public DescriptorHeap<RTDescriptor> {
    public:
        RTDescriptorHeap(const Device* device, uint32_t capacity);
        ~RTDescriptorHeap() = default;

        const RTDescriptor& EmplaceRTDescriptor(const TextureResource& texture, std::optional<ResourceFormat::Color> shaderVisibleFormat = std::nullopt);

    private:
        D3D12_RENDER_TARGET_VIEW_DESC ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc) const;
    };



    class DSDescriptorHeap : public DescriptorHeap<DSDescriptor> {
    public:
        DSDescriptorHeap(const Device* device, uint32_t capacity);
        ~DSDescriptorHeap() = default;

        const DSDescriptor& EmplaceDSDescriptor(const TextureResource& texture);

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

        uint32_t RangeCapacity() const;
        uint32_t RangeStartIndex(Range range) const;

        /*  template <class T> const CBDescriptor& EmplaceDescriptorForConstantBuffer(const BufferResource<T>& resource, std::optional<uint64_t> explicitStride = nullopt);
          template <class T> const SRDescriptor& EmplaceDescriptorForStructuredBuffer(const BufferResource<T>& resource);
          template <class T> const UADescriptor& EmplaceDescriptorForUnorderedAccessBuffer(const BufferResource<T>& resource);*/

        const SRDescriptor& EmplaceSRDescriptor(const TextureResource& resource, std::optional<ResourceFormat::Color> concreteFormat = std::nullopt);
        const UADescriptor& EmplaceUADescriptor(const TextureResource& resource, std::optional<ResourceFormat::Color> concreteFormat = std::nullopt);

    private:
        D3D12_SHADER_RESOURCE_VIEW_DESC ResourceToSRVDescription(const D3D12_RESOURCE_DESC& resourceDesc, std::optional<ResourceFormat::Color> explicitFormat = std::nullopt) const;
        D3D12_UNORDERED_ACCESS_VIEW_DESC ResourceToUAVDescription(const D3D12_RESOURCE_DESC& resourceDesc, std::optional<ResourceFormat::Color> explicitFormat = std::nullopt) const;

        Range RangeTypeForTexture(const TextureResource& texture) const;
        Range UARangeTypeForTexture(const TextureResource& texture) const;
    };

}

#include "DescriptorHeap.inl"

