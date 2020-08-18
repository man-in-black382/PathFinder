#include "Fence.hpp"
#include "Utils.h"

#include <comdef.h>

#include "../Foundation/Assert.hpp"

namespace HAL
{
  
    Fence::Fence(const Device& device)
    {
        ThrowIfFailed(device.D3DDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    }

    uint64_t Fence::IncrementExpectedValue()
    {
        return ++mExpectedValue;
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
        uint64_t completedValue = CompletedValue();

        if (completedValue == UINT64_MAX)
        {
            Microsoft::WRL::ComPtr<ID3D12Device5> device;
            mFence->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
            HRESULT removeReason = device->GetDeviceRemovedReason();
            _com_error error{ removeReason };
            assert_format(false, "Fence Completed Value indicates Device Removal: ", error.ErrorMessage());
        }

        uint8_t framesInFlight = mExpectedValue - completedValue;

        if (framesInFlight < allowedSimultaneousFramesCount) return;

        HANDLE eventHandle = CreateEventEx(nullptr, nullptr , false, EVENT_ALL_ACCESS);
        // Fire event when GPU hits current fence.  
        SetCompletionEventHandle(eventHandle);
        // Wait until the GPU hits current fence event is fired.
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

}

