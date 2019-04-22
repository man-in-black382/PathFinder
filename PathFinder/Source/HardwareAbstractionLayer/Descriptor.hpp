#pragma once

#include <d3d12.h>
#include <cstdint>

namespace HAL
{
	class Descriptor
	{
	public:
        Descriptor(uint32_t heapPosition);
	    ~Descriptor();

        D3D12_CPU_DESCRIPTOR_HANDLE mCPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle;
	};

    //class CPUDescriptor : public Descriptor {
    //protected:
    //    
    //};

    //class GPUDescriptor : public CPUDescriptor {
    //protected:
    //    
    //};

    class RTDescriptor : public Descriptor {
    public:

    };

    class DSDescriptor : public Descriptor {
        //D3D12_DEPTH_STENCIL_VIEW_DESC
    };

    class CBDescriptor : public Descriptor {
        
    };

    class SRDescriptor : public Descriptor {

    };

    class UADescriptor : public Descriptor {

    };

    class SamplerDescriptor : public Descriptor {

    };
}

