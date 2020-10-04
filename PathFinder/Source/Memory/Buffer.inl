#include "../Foundation/MemoryUtils.hpp"
#include "../Foundation/Assert.hpp"

namespace Memory
{

    template <class Element>
    uint64_t Buffer::Capacity(uint64_t elementAlignment) const
    {
        return mBufferPtr ?
            mBufferPtr->ElementCapacity<Element>(elementAlignment) :
            mUploadBuffers.front().first->ElementCapacity<Element>(elementAlignment);
    }

}
