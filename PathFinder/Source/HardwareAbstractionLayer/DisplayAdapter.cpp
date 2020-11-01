#include "DisplayAdapter.hpp"
#include "Utils.h"

namespace HAL
{
    
    DisplayAdapter::DisplayAdapter(const Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter) 
        : mAdapter(adapter) 
    {
        RefetchDisplaysIfNeeded();
    }

    void DisplayAdapter::RefetchDisplaysIfNeeded()
    {
        bool refetchNeeded = !mDXGIFactory || !mDXGIFactory->IsCurrent();

        if (!refetchNeeded)
            return;

        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&mDXGIFactory)));

        mConnectedDisplays.clear();

        UINT i = 0;
        Microsoft::WRL::ComPtr<IDXGIOutput> currentOutput;
        float bestIntersectArea = -1;

        while (mAdapter->EnumOutputs(i, &currentOutput) != DXGI_ERROR_NOT_FOUND)
        {
            Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
            ThrowIfFailed(currentOutput.As(&output6));

            DXGI_OUTPUT_DESC1 desc;
            ThrowIfFailed(output6->GetDesc1(&desc));

            mConnectedDisplays.emplace_back(desc);
            i++;
        }
    }

}
