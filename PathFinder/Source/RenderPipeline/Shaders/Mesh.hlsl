#ifndef _Mesh__
#define _Mesh__

#include "Vertices.hlsl"

struct MeshInstance
{
    float4x4 ModelMatrix;
    float4x4 PrevModelMatrix;
    float4x4 NormalMatrix;
    uint MaterialIndex;
    uint UnifiedVertexBufferOffset;
    uint UnifiedIndexBufferOffset;
    uint IndexCount;
    bool HasTangentSpace;
    bool IsDoubleSided;
};

static const uint MaterialTypeCookTorrance = 0;
static const uint MaterialTypeEmissive = 1;

struct Material
{
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex;
    uint MetalnessMapIndex;
    // 16 byte boundary
    uint DisplacementMapIndex;
    uint DistanceFieldIndex;
    uint LTC_LUT_MatrixInverse_Specular_Index;
    uint LTC_LUT_Matrix_Specular_Index;
    // 16 byte boundary
    uint LTC_LUT_Terms_Specular_Index;
    uint LTC_LUT_MatrixInverse_Diffuse_Index;
    uint LTC_LUT_Matrix_Diffuse_Index;
    uint LTC_LUT_Terms_Diffuse_Index;
    // 16 byte boundary
    uint LTC_LUT_TextureSize;
    uint SamplerIndex;
    uint HasNormalMap;
    float IOROverride;
    // 16 byte boundary
    float3 DiffuseAlbedoOverride;
    float RoughnessOverride;
    // 16 byte boundary
    float3 SpecularAlbedoOverride;
    float MetalnessOverride;
    // 16 byte boundary
    float3 TransmissionFilter;
    float TranslucencyOverride;
};

#endif