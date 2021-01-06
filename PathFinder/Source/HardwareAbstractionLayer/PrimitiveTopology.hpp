#pragma once

#include <cstdint>
#include <d3d12.h>

namespace HAL
{
   
    enum class PrimitiveTopology
    {
        LineList, PointList, TriangleList, TriangleStrip
    };

    D3D12_PRIMITIVE_TOPOLOGY_TYPE D3DPrimitiveTopologyType(PrimitiveTopology topology);
    D3D12_PRIMITIVE_TOPOLOGY D3DPrimitiveTopology(PrimitiveTopology topology);

}

