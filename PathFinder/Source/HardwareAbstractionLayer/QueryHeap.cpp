#include "QueryHeap.hpp"
#include "Utils.h"

namespace HAL
{

    QueryHeap::QueryHeap(const Device& device, uint64_t size, QueryType type)
        : mSize{ size }, mType{ type }
    {
        D3D12_QUERY_HEAP_TYPE d3dType{};

        switch (type)
        {
        case QueryType::Timestamp: d3dType = D3D12_QUERY_HEAP_TYPE_TIMESTAMP; break;
        default: assert_format(false, "Unsupported query type"); break;
        }

        D3D12_QUERY_HEAP_DESC queryHeapDesc{};
        queryHeapDesc.Count = size;
        queryHeapDesc.Type = d3dType;
        queryHeapDesc.NodeMask = device.NodeMask();
        ThrowIfFailed(device.D3DDevice()->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&mQueryHeap)));
    }

    D3D12_QUERY_TYPE QueryHeap::D3DQueryType() const
    {
        D3D12_QUERY_TYPE d3dType{};

        switch (mType)
        {
        case QueryType::Timestamp: d3dType = D3D12_QUERY_TYPE_TIMESTAMP; break;
        default: assert_format(false, "Unsupported query type");
        }

        return d3dType;
    }

}
