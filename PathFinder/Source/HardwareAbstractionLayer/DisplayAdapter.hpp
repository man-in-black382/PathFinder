#pragma once

#include <dxgi.h>
#include <wrl.h>

namespace HAL
{
	class DisplayAdapter
	{
	private:
        friend class DisplayAdapterFetcher;

        DisplayAdapter(Microsoft::WRL::ComPtr<IDXGIAdapter> adapter);
	
        Microsoft::WRL::ComPtr<IDXGIAdapter> mAdapter;

    public:
        const auto COMPtr() const { return mAdapter; }
	};
}

