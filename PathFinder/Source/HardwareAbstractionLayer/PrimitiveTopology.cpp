#include "PrimitiveTopology.hpp"

#include <type_traits>

namespace HAL
{

    D3D12_PRIMITIVE_TOPOLOGY_TYPE D3DPrimitiveTopologyType(PrimitiveTopology topology)
    {
        switch (topology) 
        {
        case PrimitiveTopology::Line: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PrimitiveTopology::Point: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case PrimitiveTopology::Triangle: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case PrimitiveTopology::Patch: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        }
    }

    D3D12_PRIMITIVE_TOPOLOGY D3DPrimitiveTopology(PrimitiveTopology topology)
    {
        switch (topology)
        {
        case PrimitiveTopology::Line: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::Point: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::Triangle: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::Patch: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED; // Deal with patches later
        }
    }

}
