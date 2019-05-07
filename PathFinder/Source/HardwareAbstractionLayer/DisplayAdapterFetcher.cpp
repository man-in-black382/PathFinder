#include "DisplayAdapterFetcher.hpp"
#include "Utils.h"

namespace HAL
{
    DisplayAdapterFetcher::DisplayAdapterFetcher()
    {
        ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&mDXGIFactory)));
    }
    
    std::vector<DisplayAdapter> DisplayAdapterFetcher::Fetch() const
    {
        uint32_t i = 0;
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        std::vector<DisplayAdapter> adapterList;

        while (mDXGIFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            adapterList.emplace_back(DisplayAdapter(adapter));
            ++i;
        }

        return adapterList;
    }

}

