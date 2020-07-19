#include "DisplayAdapter.hpp"

namespace HAL
{
	
    DisplayAdapter::DisplayAdapter(const Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter) 
        : mAdapter(adapter) { }

}
