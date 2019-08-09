#pragma once

namespace HAL
{

    template <class T>
    UADescriptor CBSRUADescriptorHeap::EmplaceDescriptorForUnorderedAccessBuffer(const BufferResource<T>& resource)
    {
        auto index = std::underlying_type_t<Range>(Range::UABuffer);

        ValidateCapacity(index);

        RangeAllocationInfo& range = GetRange(index);

        UADescriptor descriptor{ range.CurrentCPUHandle, range.CurrentGPUHandle, range.InsertedDescriptorCount };
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = ResourceToUAVDescription(resource.D3DDescription());

        mDevice->D3DPtr()->CreateUnorderedAccessView( resource.D3DPtr(), nullptr, &desc, range.CurrentCPUHandle);

        IncrementCounters(index);

        return descriptor;
    }

    template <class T>
    SRDescriptor CBSRUADescriptorHeap::EmplaceDescriptorForStructuredBuffer(const BufferResource<T>& resource)
    {
        auto index = std::underlying_type_t<Range>(Range::SBuffer);

        ValidateCapacity(index);

        RangeAllocationInfo& range = GetRange(index);

        SRDescriptor descriptor{ range.CurrentCPUHandle, range.CurrentGPUHandle, range.InsertedDescriptorCount };
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = ResourceToSRVDescription(resource.D3DDescription());

        mDevice->D3DPtr()->CreateShaderResourceView(resource.D3DPtr(), &desc, range.CurrentCPUHandle);

        IncrementCounters(index);

        return descriptor;
    }

    template <class T>
    CBDescriptor CBSRUADescriptorHeap::EmplaceDescriptorForConstantBuffer(const BufferResource<T>& resource, std::optional<uint64_t> explicitStride)
    {
        auto index = std::underlying_type_t<Range>(Range::CBuffer);

        ValidateCapacity(index);

        RangeAllocationInfo& range = GetRange(index);

        CBDescriptor descriptor{ range.CurrentCPUHandle, range.CurrentGPUHandle, range.InsertedDescriptorCount };
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc{ resource.GPUVirtualAddress(), explicitStride.has_value() ? explicitStride.value() : sizeof(T) };

        mDevice->D3DPtr()->CreateConstantBufferView(&desc, range.CurrentCPUHandle);

        IncrementCounters(index);

        return descriptor;
    }

}

