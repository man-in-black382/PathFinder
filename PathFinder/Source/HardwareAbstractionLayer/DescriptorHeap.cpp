#include "DescriptorHeap.hpp"
#include "Utils.h"

namespace HAL
{
    DescriptorHeap::DescriptorHeap(const Device* device, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
        : mDevice(device), mIncrementSize(device->D3DPtr()->GetDescriptorHandleIncrementSize(heapType))
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.NumDescriptors = capacity;
        desc.Type = heapType;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(device->D3DPtr()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)));
        mCurrentHeapHandle = mHeap->GetCPUDescriptorHandleForHeapStart();
    }

    DescriptorHeap::~DescriptorHeap() {}



    RTDescriptorHeap::RTDescriptorHeap(const Device* device, uint32_t capacity)
        : DescriptorHeap(device, capacity, D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {}

    RTDescriptor RTDescriptorHeap::EmplaceDescriptorForResource(const ColorTextureResource& resource)
    {
        RTDescriptor descriptor{ mCurrentHeapHandle, uint32_t(mCurrentHeapHandle.ptr / mIncrementSize) };
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = descriptor.ResourceToRTVDescription(resource.D3DDescription());
        mDevice->D3DPtr()->CreateRenderTargetView(resource.D3DPtr(), &rtvDesc, mCurrentHeapHandle);
        mCurrentHeapHandle.ptr += mIncrementSize;
        return descriptor;
    }



    DSDescriptorHeap::DSDescriptorHeap(const Device* device, uint32_t capacity)
        : DescriptorHeap(device, capacity, D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {}

    DSDescriptor DSDescriptorHeap::EmplaceDescriptorForResource(const DepthStencilTextureResource& resource)
    {
        DSDescriptor descriptor{ mCurrentHeapHandle, uint32_t(mCurrentHeapHandle.ptr / mIncrementSize) };
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = descriptor.ResourceToDSVDescription(resource.D3DDescription());
        mDevice->D3DPtr()->CreateDepthStencilView(resource.D3DPtr(), &dsvDesc, mCurrentHeapHandle);
        mCurrentHeapHandle.ptr += mIncrementSize;
        return descriptor;
    }

    //CBDescriptor CBSRUADescriptorHeap::EmplaceDescriptorForConstantBufferResource(const Device& device, const Resource& resource)
    //{

    //}

    //SRDescriptor CBSRUADescriptorHeap::EmplaceDescriptorForShaderResource(const Device& device, const Resource& resource)
    //{

    //}

    //UADescriptor CBSRUADescriptorHeap::EmplaceDescriptorForUnorderedAccessResource(const Device& device, const Resource& resource)
    //{

    //}

}
