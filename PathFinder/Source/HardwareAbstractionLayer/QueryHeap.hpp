#pragma once

#include "Device.hpp"
#include "GraphicAPIObject.hpp"

namespace HAL
{

    class QueryHeap : public GraphicAPIObject
    {
    public:
        enum class QueryType
        {
            Timestamp // Add more types when necessary
        };

        QueryHeap(const Device& device, uint64_t size, QueryType type);
        QueryHeap(const QueryHeap& that) = delete;
        QueryHeap(QueryHeap&& that) = default;
        ~QueryHeap() = default;

        QueryHeap& operator=(const QueryHeap& that) = delete;
        QueryHeap& operator=(QueryHeap&& that) = default;

        D3D12_QUERY_TYPE D3DQueryType() const;

    private:
        Microsoft::WRL::ComPtr<ID3D12QueryHeap> mQueryHeap;
        QueryType mType;
        uint64_t mSize;

    public:
        inline ID3D12QueryHeap* D3DQueryHeap() const { return mQueryHeap.Get(); }
        inline auto Size() const { return mSize; }
        inline auto Type() const { return mType; }
    };

}

