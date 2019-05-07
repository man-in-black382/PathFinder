#include "ResourceBarrier.hpp"
#include "Utils.h"

namespace HAL
{

	ResourceTransitionBarrier::ResourceTransitionBarrier()
	{
		mDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		mDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	}

}

