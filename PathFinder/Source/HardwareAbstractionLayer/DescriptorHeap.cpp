#include "DescriptorHeap.hpp"
#include "Utils.h"

namespace HAL
{
    DescriptorHeap::DescriptorHeap(const Device* device, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
        : mDevice(device), mCapacity(capacity), mIncrementSize(device->D3DPtr()->GetDescriptorHandleIncrementSize(heapType))
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.NumDescriptors = capacity;
        desc.Type = heapType;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(device->D3DPtr()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)));
        mCurrentHeapHandle = mHeap->GetCPUDescriptorHandleForHeapStart();
    }

    DescriptorHeap::~DescriptorHeap() {}

    void DescriptorHeap::ValidateCapacity() const
    {
        if (mInsertedDescriptorCount >= mCapacity) {
            throw std::runtime_error("Exceeded descriptor heap's capacity");
        }
    }

    void DescriptorHeap::IncrementCounters()
    {
        mCurrentHeapHandle.ptr += mIncrementSize;
        mInsertedDescriptorCount++;
    }



    RTDescriptorHeap::RTDescriptorHeap(const Device* device, uint32_t capacity)
        : DescriptorHeap(device, capacity, D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {}

    RTDescriptor RTDescriptorHeap::EmplaceDescriptorForResource(const ColorTextureResource& resource)
    {
        ValidateCapacity();

        RTDescriptor descriptor{ mCurrentHeapHandle, mInsertedDescriptorCount };
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = descriptor.ResourceToRTVDescription(resource.D3DDescription());
        mDevice->D3DPtr()->CreateRenderTargetView(resource.D3DPtr(), &rtvDesc, mCurrentHeapHandle);

        IncrementCounters();

        return descriptor;
    }

    RTDescriptor RTDescriptorHeap::EmplaceDescriptorForResource(const TypelessTextureResource& resource, ResourceFormat::Color concreteFormat)
    {
        ValidateCapacity();

        RTDescriptor descriptor{ mCurrentHeapHandle, mInsertedDescriptorCount };

        D3D12_RESOURCE_DESC d3dDesc = resource.D3DDescription();
        d3dDesc.Format = ResourceFormat::D3DFormat(concreteFormat);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = descriptor.ResourceToRTVDescription(d3dDesc);
        mDevice->D3DPtr()->CreateRenderTargetView(resource.D3DPtr(), &rtvDesc, mCurrentHeapHandle);

        IncrementCounters();

        return descriptor;
    }



    DSDescriptorHeap::DSDescriptorHeap(const Device* device, uint32_t capacity)
        : DescriptorHeap(device, capacity, D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {}

    DSDescriptor DSDescriptorHeap::EmplaceDescriptorForResource(const DepthStencilTextureResource& resource)
    {
        ValidateCapacity();

        DSDescriptor descriptor{ mCurrentHeapHandle, mInsertedDescriptorCount };
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = descriptor.ResourceToDSVDescription(resource.D3DDescription());
        mDevice->D3DPtr()->CreateDepthStencilView(resource.D3DPtr(), &dsvDesc, mCurrentHeapHandle);
        
        IncrementCounters();

        return descriptor;
    }



    CBSRUADescriptorHeap::CBSRUADescriptorHeap(const Device* device, uint32_t capacity)
        : DescriptorHeap(device, capacity, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {}

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
