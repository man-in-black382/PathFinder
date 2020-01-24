#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <vector>
#include <tuple>
#include <optional>

#include "GraphicAPIObject.hpp"
#include "Texture.hpp"
#include "Buffer.hpp"
#include "Descriptor.hpp"
#include "Device.hpp"
#include "Utils.h"

namespace HAL
{
    template <class DescriptorT>
    class DescriptorHeap : public GraphicAPIObject
    {
    public:
        struct RangeAllocationInfo
        {
            D3D12_CPU_DESCRIPTOR_HANDLE CurrentCPUHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE CurrentGPUHandle;
            std::vector<std::unique_ptr<DescriptorT>> Descriptors;

            RangeAllocationInfo(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) :
                CurrentCPUHandle{ cpuHandle }, CurrentGPUHandle{ gpuHandle } {}
        };

        DescriptorHeap(const Device* device, uint32_t rangeCapacity, uint32_t rangeCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        virtual ~DescriptorHeap() = 0;

        uint32_t Capacity() const;

    protected:
        void ValidateCapacity(uint32_t rangeIndex) const;
        void IncrementCounters(uint32_t rangeIndex);
        
        RangeAllocationInfo& GetRange(uint32_t rangeIndex);
        const RangeAllocationInfo& GetRange(uint32_t rangeIndex) const;

        const Device* mDevice = nullptr;
        uint32_t mRangeCapacity = 0;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;

    private:
        uint32_t mIncrementSize = 0;
        std::vector<RangeAllocationInfo> mRanges;

    public:
        inline ID3D12DescriptorHeap* D3DHeap() const { return mHeap.Get(); }
    };



    class RTDescriptorHeap : public DescriptorHeap<CPUDescriptor> {
    public:
        RTDescriptorHeap(const Device* device, uint32_t capacity);
        ~RTDescriptorHeap() = default;

        const RTDescriptor& EmplaceRTDescriptor(const Texture& texture, std::optional<ColorFormat> shaderVisibleFormat = std::nullopt);

    private:
        D3D12_RENDER_TARGET_VIEW_DESC ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc) const;
    };



    class DSDescriptorHeap : public DescriptorHeap<CPUDescriptor> {
    public:
        DSDescriptorHeap(const Device* device, uint32_t capacity);
        ~DSDescriptorHeap() = default;

        const DSDescriptor& EmplaceDSDescriptor(const Texture& texture);

    private:
        D3D12_DEPTH_STENCIL_VIEW_DESC ResourceToDSVDescription(const D3D12_RESOURCE_DESC& resourceDesc) const;
    };



    class CBSRUADescriptorHeap : public DescriptorHeap<GPUDescriptor> {        
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

        const GPUDescriptor* GetDescriptor(Range range, uint32_t indexInRange) const;

        const CBDescriptor& EmplaceCBDescriptor(const Buffer& buffer, uint64_t stride);
        const SRDescriptor& EmplaceSRDescriptor(const Buffer& buffer, uint64_t stride);
        const UADescriptor& EmplaceUADescriptor(const Buffer& buffer, uint64_t stride);

        const SRDescriptor& EmplaceSRDescriptor(const Texture& texture, std::optional<ColorFormat> concreteFormat = std::nullopt);
        const UADescriptor& EmplaceUADescriptor(const Texture& texture, std::optional<ColorFormat> concreteFormat = std::nullopt);

    private:
        D3D12_SHADER_RESOURCE_VIEW_DESC ResourceToSRVDescription(
            const D3D12_RESOURCE_DESC& resourceDesc, 
            uint64_t bufferStride,
            std::optional<ColorFormat> explicitFormat = std::nullopt) const;

        D3D12_UNORDERED_ACCESS_VIEW_DESC ResourceToUAVDescription(
            const D3D12_RESOURCE_DESC& resourceDesc,
            uint64_t bufferStride,
            std::optional<ColorFormat> explicitFormat = std::nullopt) const;

        Range RangeTypeForTexture(const Texture& texture) const;
        Range UARangeTypeForTexture(const Texture& texture) const;
    };

}

#include "DescriptorHeap.inl"

