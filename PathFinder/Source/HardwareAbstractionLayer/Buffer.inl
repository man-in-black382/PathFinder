#include "../Foundation/MemoryUtils.hpp"

namespace HAL
{

    template <class Element>
    Buffer::Properties<Element>::Properties(uint64_t capacity)
        : ElementCapacity{ capacity } {}

    template <class Element>
    Buffer::Properties<Element>::Properties(uint64_t capacity, uint64_t alignment)
        : ElementCapacity{ capacity }, ElementAlighnment{ alignment } {}

    template <class Element>
    Buffer::Properties<Element>::Properties(uint64_t capacity, uint64_t alignment, ResourceState initialStates)
        : ElementCapacity{ capacity }, ElementAlighnment{ alignment }, InitialState{ initialStates }, ExpectedStates{ initialStates } {}

    template <class Element>
    Buffer::Properties<Element>::Properties(uint64_t capacity, uint64_t alignment, ResourceState initialStates, ResourceState expectedStates)
        : ElementCapacity{ capacity }, ElementAlighnment{ alignment }, InitialState{ initialStates }, ExpectedStates{ expectedStates } {}



    template <class Element>
    Buffer::Buffer(const Device& device, const Properties<Element>& properties, const Heap& heap, uint64_t heapOffset)
        : Resource(device, heap, heapOffset, ConstructResourceFormat(&device, properties), properties.InitialState, properties.ExpectedStates),
        mCPUAccessibleHeapType{ heap.CPUAccessibleType() },
        mRequestedSizeInBytes{ WidthFromProperties(properties) } {}

    template <class Element>
    Buffer::Buffer(const Device& device, const Properties<Element>& properties, std::optional<CPUAccessibleHeapType> heapType)
        : Resource(device, ConstructResourceFormat(&device, properties), heapType),
        mCPUAccessibleHeapType{ heapType },
        mRequestedSizeInBytes{ WidthFromProperties(properties) } {}

    template <class Element>
    uint64_t Buffer::ElementCapacity(uint64_t elementAlignment) const
    {
        return mRequestedSizeInBytes / Foundation::MemoryUtils::Align(sizeof(Element), elementAlignment);
    }



    template <class Element>
    ResourceFormat Buffer::ConstructResourceFormat(const Device* device, const Properties<Element>& properties)
    {
        ResourceFormat format{
            device, std::nullopt, BufferKind::Buffer,
            Geometry::Dimensions{ WidthFromProperties(properties) }
        };

        format.SetExpectedStates(properties.InitialState | properties.ExpectedStates);

        return format;
    }

    template <class Element>
    uint64_t Buffer::WidthFromProperties(const Properties<Element>& properties)
    {
        return Foundation::MemoryUtils::Align(sizeof(Element), properties.ElementAlighnment) * properties.ElementCapacity;
    }

}

