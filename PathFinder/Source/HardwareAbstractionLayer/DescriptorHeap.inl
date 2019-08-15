#pragma once

namespace HAL
{
    template <class... Descriptors>
    DescriptorHeap<Descriptors...>::DescriptorHeap(const Device* device, uint32_t rangeCapacity, uint32_t rangeCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
        : mDevice{ device }, mRangeCapacity{ rangeCapacity }, mIncrementSize{ device->D3DPtr()->GetDescriptorHandleIncrementSize(heapType) }
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.NumDescriptors = rangeCapacity * rangeCount;
        desc.Type = heapType;
        desc.NodeMask = 0;

        bool shaderVisible = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(device->D3DPtr()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)));

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = mHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle{};

        if (shaderVisible)
        {
            GPUHandle = mHeap->GetGPUDescriptorHandleForHeapStart();
        }

        for (auto rangeIdx = 0u; rangeIdx < rangeCount; rangeIdx++)
        {
            RangeAllocationInfo range{ { CPUHandle.ptr + rangeIdx * mIncrementSize }, { GPUHandle.ptr + rangeIdx * mIncrementSize }, 0 };
            mRanges.push_back(range);
        }
    }

    template <class... Descriptors>
    DescriptorHeap<Descriptors...>::~DescriptorHeap() {}

    template <class... Descriptors>
    void DescriptorHeap<Descriptors...>::ValidateCapacity(uint32_t rangeIndex) const
    {
        if (mRanges[rangeIndex].InsertedDescriptorCount >= mRangeCapacity)
        {
            throw std::runtime_error("Exceeded descriptor heap's capacity");
        }
    }

    template <class... Descriptors>
    void DescriptorHeap<Descriptors...>::IncrementCounters(uint32_t rangeIndex)
    {
        RangeAllocationInfo& range = mRanges[rangeIndex];
        range.CurrentCPUHandle.ptr += mIncrementSize;
        range.CurrentGPUHandle.ptr += mIncrementSize;
        range.InsertedDescriptorCount++;
    }

    template <class... Descriptors>
    typename DescriptorHeap<Descriptors...>::RangeAllocationInfo& DescriptorHeap<Descriptors...>::GetRange(uint32_t rangeIndex)
    {
        return mRanges[rangeIndex];
    }

    template <class... Descriptors>
    uint32_t HAL::DescriptorHeap<Descriptors...>::Capacity() const
    {
        return mRangeCapacity * mRanges.size();
    }


   /* template <class T>
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
    }*/

}

