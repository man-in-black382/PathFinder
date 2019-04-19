#pragma once

#include <dxgi.h>
#include <wrl.h>

namespace HAL
{
	class DisplayAdapter
	{
	public:
        DisplayAdapter(const Microsoft::WRL::ComPtr<IDXGIAdapter>& adapter);
	
    private:
        Microsoft::WRL::ComPtr<IDXGIAdapter> mAdapter;

    public:
        inline const auto COMPtr() const { return mAdapter; }
	};
}

