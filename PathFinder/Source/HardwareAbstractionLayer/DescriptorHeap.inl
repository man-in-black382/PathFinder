#pragma once

namespace HAL
{
    template <class T>
    CBDescriptor CBSRUADescriptorHeap::EmplaceDescriptorForConstantBufferResource(const Device& device, const BufferResource<T>& resource, uint64_t byteCount)
    {
        ValidateCapacity();

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mCurrentHeapHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ mHeap->GetGPUDescriptorHandleForHeapStart().ptr + mIncrementSize * mInsertedDescriptorCount };

        CBDescriptor descriptor{ cpuHandle, gpuHandle, mInsertedDescriptorCount };
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc{ resource.GPUVirtualAddress(), byteCount };

        mDevice->D3DPtr()->CreateDepthStencilView(resource.D3DPtr(), &desc, cpuHandle);

        IncrementCounters();

        return descriptor;
    }

    template <class T>
    CBDescriptor CBSRUADescriptorHeap::EmplaceDescriptorForConstantBufferResource(const Device& device, const BufferResource<T>& resource)
    {
        return EmplaceDescriptorForConstantBufferResource(device, resource, sizeof(T));
    }
}

