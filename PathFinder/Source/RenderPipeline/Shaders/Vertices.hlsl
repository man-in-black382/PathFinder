#ifndef _Vertices__
#define _Vertices__

struct Vertex1P1N1UV
{
    float4 Position;
    float3 Normal;
    float2 UV;
};

struct Vertex1P1N1UV1T1BT 
{
    float4 Position;
    float3 Normal;
    float2 UV;
    float3 Tangent;
    float3 Bitangent;
};

struct IndexU32
{
    uint Index;
};

#endif