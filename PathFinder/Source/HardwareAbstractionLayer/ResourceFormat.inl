#include "../Foundation/MemoryUtils.hpp"

namespace HAL
{

    template <class Element>
    BufferProperties<Element>::BufferProperties(uint64_t capacity)
        : ElementCapacity{ capacity } {}

    template <class Element>
    BufferProperties<Element>::BufferProperties(uint64_t capacity, uint64_t alignment)
        : ElementCapacity{ capacity }, ElementAlighnment{ alignment } {}

    template <class Element>
    BufferProperties<Element>::BufferProperties(uint64_t capacity, uint64_t alignment, ResourceState initialStates)
        : ElementCapacity{ capacity }, ElementAlighnment{ alignment }, InitialStateMask{ initialStates }, ExpectedStateMask{ initialStates } {}

    template <class Element>
    BufferProperties<Element>::BufferProperties(uint64_t capacity, uint64_t alignment, ResourceState initialStates, ResourceState expectedStates)
        : ElementCapacity{ capacity }, ElementAlighnment{ alignment }, InitialStateMask{ initialStates }, ExpectedStateMask{ expectedStates } {}



    template <typename BufferDataT>
    ResourceFormat::ResourceFormat(const Device* device, const BufferProperties<BufferDataT>& bufferProperties)
        : mDevice{ device },
        mResourceProperties{ BufferProperties<uint8_t>{
            Foundation::MemoryUtils::Align(sizeof(BufferDataT), bufferProperties.ElementAlighnment) * bufferProperties.ElementCapacity,
            1,
            bufferProperties.InitialStateMask,
            bufferProperties.ExpectedStateMask
        } }
    {
        auto props = GetBufferProperties();
        ResolveBufferDemensionData(props.ElementCapacity);
        SetExpectedStates(props.InitialStateMask | props.ExpectedStateMask);
    }
}

