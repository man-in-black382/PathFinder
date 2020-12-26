#include "Fence.hpp"
#include "Utils.h"

#include <comdef.h>

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

    void Fence::SetCompletionEvent(uint64_t valueToWaitFor, HANDLE eventHandle)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(valueToWaitFor, eventHandle));
    }

    void Fence::ValidateCompletedValue(uint64_t value) const
    {
        if (value == UINT64_MAX)
        {
            Microsoft::WRL::ComPtr<ID3D12Device5> device;
            mFence->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
            HRESULT removeReason = device->GetDeviceRemovedReason();
            _com_error error{ removeReason };
            assert_format(false, "Fence Completed Value indicates Device Removal: ", error.ErrorMessage());
        }
    }

}

