#include "PrimitiveTopology.hpp"


#include <type_traits>

namespace HAL
{

    D3D12_PRIMITIVE_TOPOLOGY_TYPE D3DPrimitiveTopologyType(PrimitiveTopology topology)
    {
        switch (topology) 
        {
        case PrimitiveTopology::LineList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PrimitiveTopology::PointList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case PrimitiveTopology::TriangleStrip:
        case PrimitiveTopology::TriangleList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        //case PrimitiveTopology::Patch: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;

        default: assert_format(false, "Should never be hit"); return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        }
    }

    D3D12_PRIMITIVE_TOPOLOGY D3DPrimitiveTopology(PrimitiveTopology topology)
    {
        switch (topology)
        {
        case PrimitiveTopology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default: assert_format(false, "Should never be hit"); return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        }
    }

}
