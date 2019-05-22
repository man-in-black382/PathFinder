#include "Fence.hpp"
#include "Utils.h"

namespace HAL
{
  
    Fence::Fence(const Device& device)
    {
        ThrowIfFailed(device.D3DPtr()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    }

    void Fence::IncreaseExpectedValue()
    {
        mExpectedValue++;
    }

    bool Fence::IsCompleted() const
    {
        return mFence->GetCompletedValue() >= mExpectedValue;
    }

    void Fence::SetCompletionEventHandle(HANDLE handle)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(mExpectedValue, handle));
    }

}

