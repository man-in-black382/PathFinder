#pragma once

#include <numeric>

namespace HAL
{
    template <class DescriptorT>
    DescriptorHeap<DescriptorT>::DescriptorHeap(const Device* device, const std::vector<uint64_t>& rangeCapacities, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
        : mDevice{ device }, mIncrementSize{ device->D3DDevice()->GetDescriptorHandleIncrementSize(heapType) }
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.NumDescriptors = std::accumulate(rangeCapacities.begin(), rangeCapacities.end(), 0);
        desc.Type = heapType;
        desc.NodeMask = device->NodeMask();

        bool shaderVisible = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(device->D3DDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap)));

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = mHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle{};

        if (shaderVisible)
        {
            GPUHandle = mHeap->GetGPUDescriptorHandleForHeapStart();
        }

        for (auto rangeIdx = 0u; rangeIdx < rangeCapacities.size(); rangeIdx++)
        {
            uint64_t capacity = rangeCapacities[rangeIdx];

            mRanges.emplace_back(
                D3D12_CPU_DESCRIPTOR_HANDLE{ CPUHandle.ptr + rangeIdx * mIncrementSize * capacity },
                D3D12_GPU_DESCRIPTOR_HANDLE{ GPUHandle.ptr + rangeIdx * mIncrementSize * capacity },
                capacity
            );
        }
    }

    template <class DescriptorT>
    DescriptorHeap<DescriptorT>::~DescriptorHeap() {}

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
    DescriptorAddress DescriptorHeap<DescriptorT>::GetGPUAddress(uint64_t indexInRange, uint64_t rangeIndex) const
    {
        const RangeAllocationInfo& range = GetRange(rangeIndex);
        return range.StartGPUHandle.ptr + indexInRange * mIncrementSize;
    }

    template <class DescriptorT>
    DescriptorAddress DescriptorHeap<DescriptorT>::GetCPUAddress(uint64_t indexInRange, uint64_t rangeIndex) const
    {
        const RangeAllocationInfo& range = GetRange(rangeIndex);
        return range.StartCPUHandle.ptr + indexInRange * mIncrementSize;
    }

}

