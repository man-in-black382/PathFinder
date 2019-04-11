#pragma once

#include "DisplayAdapter.hpp"

#include <dxgi.h>
#include <vector>

namespace HAL
{
	class DisplayAdapterFetcher
	{
	public:
	    DisplayAdapterFetcher();
	
	    std::vector<DisplayAdapter> Fetch() const;
	
	private:
	    Microsoft::WRL::ComPtr<IDXGIFactory> mDXGIFactory;
	};
}

