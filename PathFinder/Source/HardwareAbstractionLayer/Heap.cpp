#include "Heap.hpp"

#include "../Foundation/MemoryUtils.hpp"

namespace HAL
{

    Heap::Heap(const Device& device, uint64_t size, HeapAliasingGroup aliasingGroup, std::optional<CPUAccessibleHeapType> cpuAccessibleType)
        : mAlighnedSize{ Foundation::MemoryUtils::Align(size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) }
    {
        D3D12_HEAP_DESC desc{};

        desc.Flags = D3D12_HEAP_FLAG_NONE;
        desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        desc.SizeInBytes = mAlighnedSize;

        desc.Properties.CreationNodeMask = 0;
        desc.Properties.VisibleNodeMask = 0;

        switch (aliasingGroup)
        {
        case HeapAliasingGroup::RTDSTextures: desc.Flags = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES; break;
        case HeapAliasingGroup::NonRTDSTextures: desc.Flags = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES; break;
        case HeapAliasingGroup::Buffers: desc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES; break;
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
