#pragma once

#include <wrl.h>
#include <d3d12.h>

namespace HAL
{
	//class 

	class ResourceBarrier {
	protected:
		D3D12_RESOURCE_BARRIER mDesc;
	};

	class ResourceTransitionBarrier : public ResourceBarrier
	{
    public:
		ResourceTransitionBarrier();

	private:
		

	public:
	   // inline const auto COMPtr() const { return mAdapter; }
	};
}

