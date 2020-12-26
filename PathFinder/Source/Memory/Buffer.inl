#include <Foundation/MemoryUtils.hpp>


namespace Memory
{

    template <class Element>
    uint64_t Buffer::Capacity(uint64_t elementAlignment) const
    {
        return HALBuffer()->ElementCapacity<Element>(elementAlignment);
    }

}
