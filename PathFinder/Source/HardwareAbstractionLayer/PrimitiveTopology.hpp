#pragma once

#include <cstdint>
#include <d3d12.h>

namespace HAL
{
   
	enum class PrimitiveTopology {
		Line, Point, Triangle, Patch
	};

    D3D12_PRIMITIVE_TOPOLOGY_TYPE D3DPrimitiveTopology(PrimitiveTopology topology);

}

