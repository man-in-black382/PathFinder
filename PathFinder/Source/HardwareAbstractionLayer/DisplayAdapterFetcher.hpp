#pragma once

#include "DisplayAdapter.hpp"

#include <dxgi1_2.h>
#include <vector>

namespace HAL
{
	class DisplayAdapterFetcher
	{
	public:
	    DisplayAdapterFetcher();

	private:
	    Microsoft::WRL::ComPtr<IDXGIFactory2> mDXGIFactory;
		std::unique_ptr<DisplayAdapter> mWARPAdapter;
		std::vector<DisplayAdapter> mHardwareAdapters;

	public:
		inline const auto& HardwareAdapters() const { return mHardwareAdapters; };
		const DisplayAdapter* WARPAdapter() const { return mWARPAdapter.get(); }
		DisplayAdapter& GetHardwareAdapter(uint32_t idx) { return mHardwareAdapters[idx]; }
	};
}

