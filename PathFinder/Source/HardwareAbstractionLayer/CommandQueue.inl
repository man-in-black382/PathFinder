#pragma once

#include "../Foundation/Assert.hpp"

namespace HAL
{
  
    template <class CommandListT>
    void CommandQueue::ExecuteCommandListsInternal(const CommandListT* const* lists, uint64_t count)
    {
        std::vector<const ID3D12CommandList*> d3dCmdLists;
        d3dCmdLists.resize(count);
        for (auto i = 0u; i < count; ++i)
        {
            d3dCmdLists[i] = (*(lists + i))->D3DList();
        }

        mQueue->ExecuteCommandLists(count, (ID3D12CommandList* const*)d3dCmdLists.data());
    }

}

