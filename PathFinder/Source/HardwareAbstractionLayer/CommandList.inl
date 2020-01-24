#pragma once

#include "../Foundation/Assert.hpp"

namespace HAL
{
  
    template <class T>
    void ComputeCommandListBase::SetComputeRootConstants(const T& constants, uint32_t rootParameterIndex)
    {
        mList->SetComputeRoot32BitConstants(rootParameterIndex, sizeof(T) / 4, &constants, 0);
    }

    template <class T>
    void GraphicsCommandListBase::SetGraphicsRootConstants(const T& constants, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRoot32BitConstants(rootParameterIndex, sizeof(T) / 4, &constants, 0);
    }

}

