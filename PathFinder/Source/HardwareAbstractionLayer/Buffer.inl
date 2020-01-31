namespace HAL
{

    template <class Element>
    Buffer::Properties<Element>::Properties(uint64_t capacity) : ElementCapacity{ capacity } {}

    template <class Element>
    Buffer::Properties<Element>::Properties(uint64_t capacity, uint64_t alignment)
        : ElementCapacity{ capacity }, ElementAlighnment{ alignment } {}

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
        mCPUAccessibleHeapType{ heap.CPUAccessibleType() } {}

    template <class Element>
    Buffer::Buffer(const Device& device, const Properties<Element>& properties, std::optional<CPUAccessibleHeapType> heapType)
        : Resource(device, ConstructResourceFormat(&device, properties), heapType),
        mCPUAccessibleHeapType{ heapType } {}



    template <class Element>
    ResourceFormat Buffer::ConstructResourceFormat(const Device* device, const Properties<Element>& properties)
    {
        ResourceFormat format{
            device, std::nullopt, BufferKind::Buffer,
            Geometry::Dimensions{ MemoryUtils::Align(sizeof(Element), properties.ElementAlighnment) * properties.ElementCapacity }
        };

        format.SetExpectedStates(properties.InitialState | properties.ExpectedStates);

        return format;
    }

}

