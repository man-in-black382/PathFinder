#include "DisplayAdapterFetcher.hpp"
#include "Utils.h"

namespace HAL
{

    DisplayAdapterFetcher::DisplayAdapterFetcher()
    {
        ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&mDXGIFactory)));

        uint32_t i = 0;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        while (mDXGIFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                mWARPAdapter = std::make_unique<DisplayAdapter>(adapter);
            }
            else
            {
                mHardwareAdapters.emplace_back(adapter);
            }
            
            ++i;
        }
    }

}

