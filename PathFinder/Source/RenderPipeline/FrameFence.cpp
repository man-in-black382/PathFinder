#include "FrameFence.hpp"

namespace PathFinder
{
  
    FrameFence::FrameFence(const HAL::Device& device)
        : mFence{ device } {}

    void FrameFence::StallCurrentThreadUntilCompletion(uint8_t allowedSimultaneousFramesCount)
    {
        uint64_t completedValue = mFence.CompletedValue();
        uint8_t framesInFlight = mFence.ExpectedValue()- completedValue;

        mFence.ValidateCompletedValue(completedValue);

        if (framesInFlight < allowedSimultaneousFramesCount) 
            return;

        HANDLE eventHandle = CreateEventEx(nullptr, nullptr , false, EVENT_ALL_ACCESS);

        // Fire event when GPU hits current fence.  
        // Wait for oldest value.
        UINT64 valueToWaitFor = mFence.ExpectedValue() - (allowedSimultaneousFramesCount - 1);
        mFence.SetCompletionEvent(valueToWaitFor, eventHandle);

        // Wait until the GPU hits current fence event is fired.
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

}

