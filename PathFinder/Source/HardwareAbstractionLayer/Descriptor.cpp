#include "Descriptor.hpp"
#include "Utils.h"

namespace HAL
{

    CPUDescriptor::CPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, uint32_t indexInHeap)
        : mCPUHandle(cpuHandle), mIndexInHeap(indexInHeap) {}

    CPUDescriptor::~CPUDescriptor() {}

    GPUDescriptor::GPUDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, uint32_t indexInHeap)
        : CPUDescriptor(cpuHandle, indexInHeap), mGPUHandle(gpuHandle) {}

    D3D12_RENDER_TARGET_VIEW_DESC RTDescriptor::ResourceToRTVDescription(const D3D12_RESOURCE_DESC& resourceDesc)
    {
        D3D12_RENDER_TARGET_VIEW_DESC desc;

        switch (resourceDesc.Dimension) {
        case D3D12_RESOURCE_DIMENSION_BUFFER: 
            desc.ViewDimension = D3D12_RTV_DIMENSION_BUFFER;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            desc.ViewDimension = resourceDesc.DepthOrArraySize > 1 ? D3D12_RTV_DIMENSION_TEXTURE1DARRAY : D3D12_RTV_DIMENSION_TEXTURE1D; 
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            desc.ViewDimension = resourceDesc.DepthOrArraySize > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DARRAY : D3D12_RTV_DIMENSION_TEXTURE2D;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            break;
        }

        desc.Format = resourceDesc.Format;

        return desc;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC DSDescriptor::ResourceToDSVDescription(const D3D12_RESOURCE_DESC& resourceDesc)
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC desc;
        
        switch (resourceDesc.Dimension) {
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            desc.ViewDimension = resourceDesc.DepthOrArraySize > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DARRAY : D3D12_DSV_DIMENSION_TEXTURE2D;
            break;
        default:
            throw std::invalid_argument("Unsupported depth resource dimension");
        }

        desc.Format = resourceDesc.Format;

        return desc;
    }

}
