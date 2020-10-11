#include <Foundation/MemoryUtils.hpp>

namespace HAL
{

    template <class Element>
    uint64_t Buffer::ElementCapacity(uint64_t elementAlignment) const
    {
        return mProperties.Size / Foundation::MemoryUtils::Align(sizeof(Element), elementAlignment);
    }

}

