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

    void Fence::StallCurrentThreadUntilCompletion(uint8_t allowedSimultaneousFramesCount)
    {
        uint8_t framesInFlight = ExpectedValue() - CompletedValue();

        if (framesInFlight < allowedSimultaneousFramesCount) {
            return;
        }

        //OutputDebugString("Stalling thread \n");

        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        // Fire event when GPU hits current fence.  
        SetCompletionEventHandle(eventHandle);
        // Wait until the GPU hits current fence event is fired.
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

}

