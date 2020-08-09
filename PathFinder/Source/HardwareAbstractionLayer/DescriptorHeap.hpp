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
#include "Types.hpp"
#include "Sampler.hpp"

namespace HAL
{

    template <class DescriptorT>
    class DescriptorHeap : public GraphicAPIObject
    {
    public:
        struct RangeAllocationInfo
        {
            D3D12_CPU_DESCRIPTOR_HANDLE StartCPUHandle{ 0 };
            D3D12_GPU_DESCRIPTOR_HANDLE StartGPUHandle{ 0 };
            uint64_t RangeCapacity = 0;

            RangeAllocationInfo(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, uint64_t capacity) :
                StartCPUHandle{ cpuHandle }, StartGPUHandle{ gpuHandle }, RangeCapacity{ capacity } {}
        };

        DescriptorHeap(const Device* device, const std::vector<uint64_t>& rangeCapacities, D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        virtual ~DescriptorHeap() = 0;

    protected:        
        RangeAllocationInfo& GetRange(uint32_t rangeIndex);
        const RangeAllocationInfo& GetRange(uint32_t rangeIndex) const;
        DescriptorAddress GetCPUAddress(uint64_t indexInRange, uint64_t rangeIndex) const;
        DescriptorAddress GetGPUAddress(uint64_t indexInRange, uint64_t rangeIndex) const;

        const Device* mDevice = nullptr;
        uint32_t mIncrementSize = 0;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;

    private:
        std::vector<RangeAllocationInfo> mRanges;

    public:
        inline ID3D12DescriptorHeap* D3DHeap() const { return mHeap.Get(); }
    };



    class RTDescriptorHeap : public DescriptorHeap<CPUDescriptor> {
    public:
        RTDescriptorHeap(const Device* device, uint32_t capacity);
        ~RTDescriptorHeap() = default;

        const RTDescriptor EmplaceRTDescriptor(uint64_t indexInHeap, const Texture& texture, uint8_t mipLevel = 0, std::optional<ColorFormat> shaderVisibleFormat = std::nullopt);

    private:
        D3D12_RENDER_TARGET_VIEW_DESC ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc, uint8_t mipLevel) const;
    };



    class DSDescriptorHeap : public DescriptorHeap<CPUDescriptor> {
    public:
        DSDescriptorHeap(const Device* device, uint32_t capacity);
        ~DSDescriptorHeap() = default;

        const DSDescriptor EmplaceDSDescriptor(uint64_t indexInHeap, const Texture& texture);

    private:
        D3D12_DEPTH_STENCIL_VIEW_DESC ResourceToDSVDescription(const D3D12_RESOURCE_DESC& resourceDesc) const;
    };



    class CBSRUADescriptorHeap : public DescriptorHeap<GPUDescriptor> {        
    public:
        enum class Range : uint8_t
        {
            ShaderResource = 0, UnorderedAccess = 1, ConstantBuffer = 2
        };

        CBSRUADescriptorHeap(const Device* device, uint64_t shaderResourceRangeCapacity, uint64_t unorderedAccessRangeCapacity, uint64_t constantBufferRangeCapacity );
        ~CBSRUADescriptorHeap() = default;

        const CBDescriptor EmplaceCBDescriptor(uint64_t indexInHeapRange, const Buffer& buffer, uint64_t stride);
        const SRDescriptor EmplaceSRDescriptor(uint64_t indexInHeapRange, const Buffer& buffer, uint64_t stride);
        const UADescriptor EmplaceUADescriptor(uint64_t indexInHeapRange, const Buffer& buffer, uint64_t stride);

        const SRDescriptor EmplaceSRDescriptor(uint64_t indexInHeapRange, const Texture& texture, std::optional<ColorFormat> concreteFormat = std::nullopt);
        const UADescriptor EmplaceUADescriptor(uint64_t indexInHeapRange, const Texture& texture, uint8_t mipLevel = 0, std::optional<ColorFormat> concreteFormat = std::nullopt);

        DescriptorAddress RangeStartGPUAddress(Range range) const;

    private:
        D3D12_SHADER_RESOURCE_VIEW_DESC ResourceToSRVDescription(
            const D3D12_RESOURCE_DESC& resourceDesc, 
            uint64_t bufferStride,
            std::optional<ColorFormat> explicitFormat = std::nullopt) const;

        D3D12_SHADER_RESOURCE_VIEW_DESC BufferToAccelerationStructureDescription(const Buffer& buffer) const;

        D3D12_UNORDERED_ACCESS_VIEW_DESC ResourceToUAVDescription(
            const D3D12_RESOURCE_DESC& resourceDesc,
            uint64_t bufferStride,
            uint8_t mipLevel = 0,
            std::optional<ColorFormat> explicitFormat = std::nullopt) const;
    };



    class SamplerDescriptorHeap : public DescriptorHeap<GPUDescriptor> {
    public:
        SamplerDescriptorHeap(const Device* device, uint32_t capacity);
        ~SamplerDescriptorHeap() = default;

        const SamplerDescriptor EmplaceSamplerDescriptor(uint64_t indexInHeap, const Sampler& sampler);

        DescriptorAddress StartGPUAddress() const;

    private:
    };

}

#include "DescriptorHeap.inl"

