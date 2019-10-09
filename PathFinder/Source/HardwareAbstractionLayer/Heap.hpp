#pragma once

#include "Device.hpp"

#include <optional>

namespace HAL
{

    enum class HeapAliasingGroup
    {
        RTDSTextures, NonRTDSTextures, Buffers
    };

    enum class CPUAccessibleHeapType
    {
        Upload, Readback
    };

    class Heap
    {
    public:
        Heap(const Device& device, uint64_t size, HeapAliasingGroup aliasingGroup, std::optional<CPUAccessibleHeapType> cpuAccessibleType = std::nullopt);

    private:
        Microsoft::WRL::ComPtr<ID3D12Heap> mHeap;
        uint64_t mAlighnedSize;

    public:
        inline ID3D12Heap* D3DHeap() const { return mHeap.Get(); }
        inline auto AlighnedSize() const { return mAlighnedSize; }
    };

}

