#pragma once

namespace PathFinder
{

    template <class T>
    const HAL::SRDescriptor& ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded(const HAL::Buffer<T>* buffer, std::optional<uint64_t> explicitStride)
    {
        if (auto descriptor = GetSRDescriptor(buffer)) return *descriptor;

        const HAL::SRDescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceSRDescriptor(*buffer, explicitStride);
        mDescriptors[buffer].ImplicitlyTypedRTSRUA.SRDescriptor = &descriptor;

        return descriptor;
    }

    template <class T>
    const HAL::UADescriptor& ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded(const HAL::Buffer<T>* buffer, std::optional<uint64_t> explicitStride)
    {
        if (auto descriptor = GetUADescriptor(buffer)) return *descriptor;

        const HAL::UADescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceUADescriptor(*buffer, explicitStride);
        mDescriptors[buffer].ImplicitlyTypedRTSRUA.UADescriptor = &descriptor;

        return descriptor;
    }

    template <class T>
    const HAL::CBDescriptor& ResourceDescriptorStorage::EmplaceCBDescriptorIfNeeded(const HAL::Buffer<T>* buffer, std::optional<uint64_t> explicitStride)
    {
        if (auto descriptor = GetCBDescriptor(buffer)) return *descriptor;

        const HAL::CBDescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceCBDescriptor(*buffer, explicitStride);
        mDescriptors[buffer].DSCB.CBDescriptor = &descriptor;

        return descriptor;
    }

}