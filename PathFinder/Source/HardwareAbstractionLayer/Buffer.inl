#include "../Foundation/MemoryUtils.hpp"

namespace HAL
{

    template <class Element>
    Buffer::Buffer(const Device& device, const BufferProperties<Element>& properties, const Heap& heap, uint64_t heapOffset)
        : Resource(device, heap, heapOffset, ResourceFormat{ &device, properties }),
        mCPUAccessibleHeapType{ heap.CPUAccessibleType() },
        mProperties{ Foundation::MemoryUtils::Align(sizeof(Element), properties.ElementAlighnment) * properties.ElementCapacity, 1 } {}

    template <class Element>
    Buffer::Buffer(const Device& device, const BufferProperties<Element>& properties, std::optional<CPUAccessibleHeapType> heapType)
        : Resource(device, ResourceFormat{ &device, properties }, heapType),
        mCPUAccessibleHeapType{ heapType },
        mProperties{ Foundation::MemoryUtils::Align(sizeof(Element), properties.ElementAlighnment) * properties.ElementCapacity, 1 } {}

    template <class Element>
    uint64_t Buffer::ElementCapacity(uint64_t elementAlignment) const
    {
        return mProperties.ElementCapacity / Foundation::MemoryUtils::Align(sizeof(Element), elementAlignment);
    }

}

