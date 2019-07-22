#pragma once

#include <d3d12.h>
#include <cstdint>

#include "BufferResource.hpp"

namespace HAL
{

    class CPUDescriptor {
    public:
        CPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, uint32_t indexInHeap);
        virtual ~CPUDescriptor() = 0;

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE mCPUHandle;
        uint32_t mIndexInHeap;

    public:
        inline const auto& CPUHandle() const { return mCPUHandle; }
        inline const auto& IndexInHeap() const { return mIndexInHeap; }
    };

    class GPUDescriptor : public CPUDescriptor {
    public:
        GPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, uint32_t indexInHeap);

    private:
        D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle;

    public:
        inline const auto& GPUHandle() const { return mGPUHandle; }
    };


    class RTDescriptor : public CPUDescriptor 
    {
    public:
        using CPUDescriptor::CPUDescriptor;
        ~RTDescriptor() = default;

        D3D12_RENDER_TARGET_VIEW_DESC ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc);
    };

    class DSDescriptor : public CPUDescriptor
    {
    public:
        using CPUDescriptor::CPUDescriptor;
        ~DSDescriptor() = default;

        D3D12_DEPTH_STENCIL_VIEW_DESC ResourceToDSVDescription(const D3D12_RESOURCE_DESC& resourceDesc);
    };

    class CBDescriptor : public GPUDescriptor
    {
    public:
        ~CBDescriptor() = default;
    };

    class SRDescriptor : public GPUDescriptor
    {
    public:
        ~SRDescriptor() = default;
    };

    class UADescriptor : public GPUDescriptor
    {
    public:
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
        VertexBufferDescriptor(const BufferResource<Vertex>& vertexBuffer);

    private:
        D3D12_VERTEX_BUFFER_VIEW mDescriptor{};

    public:
        inline const auto& D3DDescriptor() const { return mDescriptor; }
    };

    template <class Vertex>
    VertexBufferDescriptor::VertexBufferDescriptor(const BufferResource<Vertex>& vertexBuffer)
    {
        mDescriptor.BufferLocation = vertexBuffer.D3DPtr()->GetGPUVirtualAddress();
        mDescriptor.SizeInBytes = vertexBuffer.D3DDescription().Width;
        mDescriptor.StrideInBytes = vertexBuffer.PaddedElementSize();
    }



    class IndexBufferDescriptor
    {
    public:
        template <class Index>
        IndexBufferDescriptor(const BufferResource<Index>& indexBuffer, ResourceFormat::Color format);

    private:
        D3D12_INDEX_BUFFER_VIEW mDescriptor{};

    public:
        inline const auto& D3DDescriptor() const { return mDescriptor; }
    };

    template <class Index>
    IndexBufferDescriptor::IndexBufferDescriptor(const BufferResource<Index>& indexBuffer, ResourceFormat::Color format)
    {
        mDescriptor.BufferLocation = indexBuffer.D3DPtr()->GetGPUVirtualAddress();
        mDescriptor.SizeInBytes = indexBuffer.D3DDescription().Width;
        mDescriptor.Format = ResourceFormat::D3DFormat(format);
    }

}

