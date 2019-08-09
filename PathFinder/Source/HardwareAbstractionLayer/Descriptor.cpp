#include "Descriptor.hpp"
#include "Utils.h"

namespace HAL
{

    CPUDescriptor::CPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, uint32_t indexInHeap)
        : mCPUHandle(cpuHandle), mIndexInHeap(indexInHeap) {}

    CPUDescriptor::~CPUDescriptor() {}

    GPUDescriptor::GPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, uint32_t indexInHeap)
        : CPUDescriptor(cpuHandle, indexInHeap), mGPUHandle(gpuHandle) {}

}
