#include "Heap.hpp"

#include <Foundation/MemoryUtils.hpp>

namespace HAL
{

    Heap::Heap(const Device& device, uint64_t size, HeapAliasingGroup aliasingGroup, std::optional<CPUAccessibleHeapType> cpuAccessibleType)
        : mAlighnedSize{ Foundation::MemoryUtils::Align(size, device.MandatoryHeapAlignment()) }, mCPUAccessibleType{ cpuAccessibleType }
    {
        D3D12_HEAP_DESC desc{};

        desc.Flags = D3D12_HEAP_FLAG_NONE;
        desc.Alignment = device.MandatoryHeapAlignment();
        desc.SizeInBytes = mAlighnedSize;

        desc.Properties.CreationNodeMask = device.NodeMask();
        desc.Properties.VisibleNodeMask = device.NodeMask();

        switch (aliasingGroup)
        {
        case HeapAliasingGroup::RTDSTextures: desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES; break;
        case HeapAliasingGroup::NonRTDSTextures: desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES; break;
        case HeapAliasingGroup::Buffers: desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS; break;
        case HeapAliasingGroup::Universal: desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES; break;
        }

        if (cpuAccessibleType)
        {
            switch (*cpuAccessibleType)
            {
            case CPUAccessibleHeapType::Upload: desc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD; break;
            case CPUAccessibleHeapType::Readback: desc.Properties.Type = D3D12_HEAP_TYPE_READBACK; break;
            }
        } 
        else {
            desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        }

        device.D3DDevice()->CreateHeap(&desc, IID_PPV_ARGS(mHeap.GetAddressOf()));
    }

}
