#pragma once

namespace HAL
{
    class RTDescriptor;
    class DSDescriptor;
    class CBDescriptor;
    class SRDescriptor;
    class UADescriptor;
    class SamplerDescriptor;

    template <class DescriptorT> class DescriptorHeapTypeResolver
    {
    public:
        static constexpr D3D12_DESCRIPTOR_HEAP_TYPE HeapType() { static_assert("Unsupported descriptor type"); }
    };

    template <> class DescriptorHeapTypeResolver<RTDescriptor> {
    public:
        static constexpr D3D12_DESCRIPTOR_HEAP_TYPE HeapType() { return D3D12_DESCRIPTOR_HEAP_TYPE_RTV; };
    };

    template <> class DescriptorHeapTypeResolver<DSDescriptor> {
    public:
        static constexpr D3D12_DESCRIPTOR_HEAP_TYPE HeapType() { return D3D12_DESCRIPTOR_HEAP_TYPE_DSV; };
    };

    template <> class DescriptorHeapTypeResolver<CBDescriptor> {
    public:
        static constexpr D3D12_DESCRIPTOR_HEAP_TYPE HeapType() { return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; };
    };

    template <> class DescriptorHeapTypeResolver<SRDescriptor> {
    public:
        static constexpr D3D12_DESCRIPTOR_HEAP_TYPE HeapType() { return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; };
    };

    template <> class DescriptorHeapTypeResolver<UADescriptor> {
    public:
        static constexpr D3D12_DESCRIPTOR_HEAP_TYPE HeapType() { return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; };
    };

    template <> class DescriptorHeapTypeResolver<SamplerDescriptor> {
    public:
        static constexpr D3D12_DESCRIPTOR_HEAP_TYPE HeapType() { return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER; };
    };
}