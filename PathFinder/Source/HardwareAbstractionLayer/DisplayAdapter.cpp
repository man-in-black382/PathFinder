#include "DisplayAdapter.hpp"

namespace HAL
{
	
    DisplayAdapter::DisplayAdapter(const Microsoft::WRL::ComPtr<IDXGIAdapter>& adapter) : mAdapter(adapter) { }

}
