#pragma once

#include <d3d12.h>
#include <cstdint>

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
        inline const auto CPUHeapPtr() const { return mCPUHandle.ptr; }
        inline const auto IndexInHeap() const { return mIndexInHeap; }
    };

    class GPUDescriptor : public CPUDescriptor {
    public:
        GPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, uint32_t indexInHeap);
        virtual ~GPUDescriptor() = 0;

    private:
        D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle;

    public:
        inline const auto GPUHeapPtr() const { return mGPUHandle.ptr; }
    };


    class RTDescriptor : public CPUDescriptor {
    public:
        using CPUDescriptor::CPUDescriptor;
        ~RTDescriptor() = default;

        D3D12_RENDER_TARGET_VIEW_DESC ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc);
    };

    class DSDescriptor : public CPUDescriptor {
    public:
        using CPUDescriptor::CPUDescriptor;
        ~DSDescriptor() = default;

        D3D12_DEPTH_STENCIL_VIEW_DESC ResourceToDSVDescription(const D3D12_RESOURCE_DESC& resourceDesc);
    };

    class CBDescriptor : public GPUDescriptor {
        
    };

    class SRDescriptor : public GPUDescriptor {

    };

    class UADescriptor : public GPUDescriptor {

    };

    class SamplerDescriptor : public GPUDescriptor {

    };

}

