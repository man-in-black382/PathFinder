#pragma once

#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>

#include "Display.hpp"

namespace HAL
{
    class DisplayAdapter
    {
    public:
        DisplayAdapter(const Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter);

        void RefetchDisplaysIfNeeded();
    
    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> mDXGIFactory;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> mAdapter;
        std::vector<Display> mConnectedDisplays;

    public:
        inline const auto D3DAdapter() const { return mAdapter.Get(); }
        inline const auto& Displays() const { return mConnectedDisplays; }
    };
}

