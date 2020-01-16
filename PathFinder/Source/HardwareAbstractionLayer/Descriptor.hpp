#pragma once

#include <d3d12.h>
#include <cstdint>

#include "Buffer.hpp"

namespace HAL
{

    class CPUDescriptor {
    public:
        CPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
        virtual ~CPUDescriptor() = 0;

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE mCPUHandle;

    public:
        inline const auto& CPUHandle() const { return mCPUHandle; }
    };

    class GPUDescriptor : public CPUDescriptor {
    public:
        GPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, uint32_t indexInHeapRange);
        virtual ~GPUDescriptor() = 0;

    private:
        D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle;
        uint32_t mIndexInHeapRange;

    public:
        inline const auto& GPUHandle() const { return mGPUHandle; }
        inline const auto& IndexInHeapRange() const { return mIndexInHeapRange; }
    };


    class RTDescriptor : public CPUDescriptor 
    {
    public:
        using CPUDescriptor::CPUDescriptor;
        ~RTDescriptor() = default;
    };

    class DSDescriptor : public CPUDescriptor
    {
    public:
        using CPUDescriptor::CPUDescriptor;
        ~DSDescriptor() = default;
    };

    class CBDescriptor : public GPUDescriptor
    {
    public:
        using GPUDescriptor::GPUDescriptor;
        ~CBDescriptor() = default;
    };

    class SRDescriptor : public GPUDescriptor
    {
    public:
        using GPUDescriptor::GPUDescriptor;
        ~SRDescriptor() = default;
    };

    class UADescriptor : public GPUDescriptor
    {
    public:
        using GPUDescriptor::GPUDescriptor;
        ~UADescriptor() = default;
    };

    class SamplerDescriptor : public GPUDescriptor
    {
    public:
        ~SamplerDescriptor() = default;
    };



    class VertexBufferDescriptor
    {
    public:
        template <class Vertex>
        VertexBufferDescriptor(const Buffer<Vertex>& vertexBuffer);

    private:
        D3D12_VERTEX_BUFFER_VIEW mDescriptor{};

    public:
        inline const auto& D3DDescriptor() const { return mDescriptor; }
    };

    

    class IndexBufferDescriptor
    {
    public:
        template <class Index>
        IndexBufferDescriptor(const Buffer<Index>& indexBuffer, ColorFormat format);

    private:
        D3D12_INDEX_BUFFER_VIEW mDescriptor{};

    public:
        inline const auto& D3DDescriptor() const { return mDescriptor; }
    };

   

}

#include "Descriptor.inl"