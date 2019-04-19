#pragma once

#include <wrl.h>
#include <d3d12.h>

namespace HAL
{
	class Fence
	{
    public:
	    Fence(const Microsoft::WRL::ComPtr<ID3D12Device>& device);
	
	private:
	    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	
	public:
	   // inline const auto COMPtr() const { return mAdapter; }
	};
}

