#ifndef _Mesh__
#define _Mesh__

struct MeshInstance
{
    float4x4 ModelMatrix;
    float4x4 NormalMatrix;
    uint MaterialIndex;
    uint UnifiedVertexBufferOffset;
    uint UnifiedIndexBufferOffset;
    uint IndexCount;
};

static const uint MaterialTypeCookTorrance = 0;
static const uint MaterialTypeEmissive = 1;

struct Material
{
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex;
    uint MetalnessMapIndex;
    uint AOMapIndex;
    uint DisplacementMapIndex;
    uint DistanceFieldIndex;
    uint LTC_LUT_0_Specular_Index;
    uint LTC_LUT_1_Specular_Index;
    uint LTC_LUT_TextureSize;
};

#endif