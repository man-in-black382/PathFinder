#pragma once

#include <dxgi.h>
#include <wrl.h>

namespace HAL
{
    class DisplayAdapter
    {
    public:
        DisplayAdapter(const Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter);
    
    private:
        Microsoft::WRL::ComPtr<IDXGIAdapter1> mAdapter;

    public:
        inline const auto D3DAdapter() const { return mAdapter.Get(); }
    };
}

