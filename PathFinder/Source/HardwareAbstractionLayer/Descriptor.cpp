#include "Descriptor.hpp"
#include "Utils.h"

namespace HAL
{

    CPUDescriptor::CPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
        : mCPUHandle(cpuHandle) {}

    CPUDescriptor::~CPUDescriptor() {}

    GPUDescriptor::GPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, uint64_t indexInHeapRange)
        : CPUDescriptor(cpuHandle), mGPUHandle{ gpuHandle }, mIndexInHeapRange{ indexInHeapRange } {}

    GPUDescriptor::~GPUDescriptor() {}

}
