#ifndef _InstanceData__
#define _InstanceData__

struct MeshInstanceData
{
    float4x4 ModelMatrix;
    float4x4 NormalMatrix;
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex;
    uint MetalnessMapIndex;
    uint AOMapIndex;
    uint DisplacementMapIndex;
    uint DistanceFieldIndex;
    uint UnifiedVertexBufferOffset;
    uint UnifiedIndexBufferOffset;
    uint IndexCount;
};

static const uint LightTypeDisk = 0;
static const uint LightTypeSphere = 1;
static const uint LightTypeRectangle = 2;

struct LightInstanceData
{
    float LuminousIntensity;
    float Width;
    float Height;
    uint LightType;
    float4 Orientation;
    float4 Position;
    float4 Color;
};

struct DiskLightPoints
{
    float3 Points[4];
};

#endif