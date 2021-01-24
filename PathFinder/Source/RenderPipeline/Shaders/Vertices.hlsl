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

static const float2 UnitQuadVertices[4] =
{
    float2(-0.5, -0.5),
    float2(-0.5, 0.5),
    float2(0.5, 0.5),
    float2(0.5, -0.5)
};

// Counter-Clockwise
static const uint UnitQuadIndices[6] = { 0, 3, 1, 3, 2, 1 };

static const float3 UnitCubeVertices[8] =
{
    float3(-0.5, -0.5, 0.5),
    float3(0.5, -0.5, 0.5),
    float3(-0.5, 0.5, 0.5),
    float3(0.5, 0.5, 0.5),
    float3(-0.5, 0.5, -0.5),
    float3(0.5, 0.5, -0.5),
    float3(-0.5, -0.5, -0.5),
    float3(0.5, -0.5, -0.5),
};

static const uint UnitCubeIndices[36] =
{
    0,1,2,
    2,1,3,
    2,3,4,
    4,3,5,
    4,5,6,
    6,5,7,
    6,7,0,
    0,7,1,
    1,7,3,
    3,7,5,
    6,0,4,
    4,0,2
};

#endif