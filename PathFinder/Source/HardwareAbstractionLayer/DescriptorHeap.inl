#pragma once

namespace HAL
{
    template <class DescriptorT>
    DescriptorHeap<DescriptorT>::DescriptorHeap(const Device* device, uint32_t rangeCapacity, uint32_t rangeCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
        : mDevice{ device }, mRangeCapacity{ rangeCapacity }, mIncrementSize{ device->D3DDevice()->GetDescriptorHandleIncrementSize(heapType) }
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.NumDescriptors = rangeCapacity * rangeCount;
        desc.Type = heapType;
        desc.NodeMask = 0;

        bool shaderVisible = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(device->D3DDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)));

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = mHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle{};

        if (shaderVisible)
        {
            GPUHandle = mHeap->GetGPUDescriptorHandleForHeapStart();
        }

        for (auto rangeIdx = 0u; rangeIdx < rangeCount; rangeIdx++)
        {
            mRanges.emplace_back(
                D3D12_CPU_DESCRIPTOR_HANDLE{ CPUHandle.ptr + rangeIdx * mIncrementSize * mRangeCapacity }, 
                D3D12_GPU_DESCRIPTOR_HANDLE{ GPUHandle.ptr + rangeIdx * mIncrementSize * mRangeCapacity }
            );
        }
    }

    template <class DescriptorT>
    DescriptorHeap<DescriptorT>::~DescriptorHeap() {}

    template <class DescriptorT>
    void DescriptorHeap<DescriptorT>::ValidateCapacity(uint32_t rangeIndex) const
    {
        if (mRanges[rangeIndex].Descriptors.size() >= mRangeCapacity)
        {
            throw std::runtime_error("Exceeded descriptor heap's capacity");
        }
    }

    template <class DescriptorT>
    void DescriptorHeap<DescriptorT>::IncrementCounters(uint32_t rangeIndex)
    {
        RangeAllocationInfo& range = mRanges[rangeIndex];
        range.CurrentCPUHandle.ptr += mIncrementSize;
        range.CurrentGPUHandle.ptr += mIncrementSize;
    }

    template <class DescriptorT>
    typename DescriptorHeap<DescriptorT>::RangeAllocationInfo& DescriptorHeap<DescriptorT>::GetRange(uint32_t rangeIndex)
    {
        return mRanges[rangeIndex];
    }

    template <class DescriptorT>
    typename const DescriptorHeap<DescriptorT>::RangeAllocationInfo& DescriptorHeap<DescriptorT>::GetRange(uint32_t rangeIndex) const
    {
        return mRanges.at(rangeIndex);
    }

    template <class DescriptorT>
    uint32_t HAL::DescriptorHeap<DescriptorT>::Capacity() const
    {
        return mRangeCapacity * mRanges.size();
    }

}

