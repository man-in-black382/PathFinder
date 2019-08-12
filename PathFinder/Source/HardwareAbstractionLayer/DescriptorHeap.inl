#pragma once

namespace HAL
{

    template <class T>
    const UADescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForUnorderedAccessBuffer(const BufferResource<T>& resource)
    {
        auto index = std::underlying_type_t<Range>(Range::UABuffer);

        ValidateCapacity(index);

        RangeAllocationInfo& range = GetRange(index);

        const UADescriptor& descriptor = std::get<DescriptorContainer<UADescriptor>>(mDescriptors).emplace_back(range.CurrentCPUHandle, range.CurrentGPUHandle, range.InsertedDescriptorCount);
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = ResourceToUAVDescription(resource.D3DDescription());

        mDevice->D3DPtr()->CreateUnorderedAccessView( resource.D3DPtr(), nullptr, &desc, range.CurrentCPUHandle);

        IncrementCounters(index);

        return descriptor;
    }

    template <class T>
    const SRDescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForStructuredBuffer(const BufferResource<T>& resource)
    {
        auto index = std::underlying_type_t<Range>(Range::SBuffer);

        ValidateCapacity(index);

        RangeAllocationInfo& range = GetRange(index);

        const SRDescriptor& descriptor = std::get<DescriptorContainer<SRDescriptor>>(mDescriptors).emplace_back(range.CurrentCPUHandle, range.CurrentGPUHandle, range.InsertedDescriptorCount);
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = ResourceToSRVDescription(resource.D3DDescription());

        mDevice->D3DPtr()->CreateShaderResourceView(resource.D3DPtr(), &desc, range.CurrentCPUHandle);

        IncrementCounters(index);

        return descriptor;
    }

    template <class T>
    const CBDescriptor& CBSRUADescriptorHeap::EmplaceDescriptorForConstantBuffer(const BufferResource<T>& resource, std::optional<uint64_t> explicitStride)
    {
        auto index = std::underlying_type_t<Range>(Range::CBuffer);

        ValidateCapacity(index);

        RangeAllocationInfo& range = GetRange(index);

        const CBDescriptor& descriptor = std::get<DescriptorContainer<CBDescriptor>>(mDescriptors).emplace_back(range.CurrentCPUHandle, range.CurrentGPUHandle, range.InsertedDescriptorCount);
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc{ resource.GPUVirtualAddress(), explicitStride.has_value() ? explicitStride.value() : sizeof(T) };

        mDevice->D3DPtr()->CreateConstantBufferView(&desc, range.CurrentCPUHandle);

        IncrementCounters(index);

        return descriptor;
    }

}

