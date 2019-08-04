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
        D3D12_RENDER_TARGET_VIEW_DESC desc{};

        switch (resourceDesc.Dimension) {
        case D3D12_RESOURCE_DIMENSION_BUFFER: 
            desc.ViewDimension = D3D12_RTV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)resourceDesc.Width;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (resourceDesc.DepthOrArraySize > 1) 
            {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray.MipSlice = 0;
                desc.Texture1DArray.FirstArraySlice = 0;
                desc.Texture1DArray.ArraySize = resourceDesc.DepthOrArraySize;
            }
            else {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = 0;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = 0;
                desc.Texture2DArray.FirstArraySlice = 0;
                desc.Texture2DArray.PlaneSlice = 0;
                desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
            }
            else {
                desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;
                desc.Texture2D.PlaneSlice = 0;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = 0;
            desc.Texture3D.FirstWSlice = 0;
            desc.Texture3D.WSize = -1; // A value of - 1 indicates all of the slices along the w axis, starting from FirstWSlice.
            break;
        }

        desc.Format = resourceDesc.Format;

        return desc;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC DSDescriptor::ResourceToDSVDescription(const D3D12_RESOURCE_DESC& resourceDesc)
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
        
        switch (resourceDesc.Dimension) {
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resourceDesc.DepthOrArraySize > 1)
            {
                desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray.MipSlice = 0;
                desc.Texture2DArray.FirstArraySlice = 0;
                desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
            }
            else {
                desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;
            }
            break;

        default:
            throw std::invalid_argument("Unsupported depth resource dimension");
        }

        desc.Format = resourceDesc.Format;

        return desc;
    }

}
