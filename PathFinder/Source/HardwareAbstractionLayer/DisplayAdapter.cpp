#include "DisplayAdapter.hpp"

namespace HAL
{
	
    DisplayAdapter::DisplayAdapter(Microsoft::WRL::ComPtr<IDXGIAdapter> adapter) : mAdapter(adapter)
    {

    }

}
