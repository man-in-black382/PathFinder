#pragma once

#include <d3d12.h>
#include <cstdint>

namespace HAL
{

    class CPUDescriptor {
    public:
        CPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE mCPUHandle;
    };

    class GPUDescriptor : public CPUDescriptor {
    public:
        GPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    private:
        D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle;
    };

    class RTDescriptor : public CPUDescriptor {
    public:
        using CPUDescriptor::CPUDescriptor;
        D3D12_RENDER_TARGET_VIEW_DESC ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc);
    };

    class DSDescriptor : public CPUDescriptor {
    public:
        using CPUDescriptor::CPUDescriptor;
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

