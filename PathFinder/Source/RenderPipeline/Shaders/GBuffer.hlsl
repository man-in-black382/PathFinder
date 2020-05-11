#ifndef _GBuffer__
#define _GBuffer__

#include "Packing.hlsl"
#include "Camera.hlsl"

static const uint GBufferTypeStandard = 0;
static const uint GBufferTypeEmissive = 1;

struct GBufferTextureIndices
{
    uint AlbedoMetalnessTextureIndex;
    uint RoughnessTextureIndex;
    uint NormalTextureIndex;
    uint MotionTextureIndex;
    // 16 byte boundary
    uint TypeAndMaterialTextureIndex;
    uint ViewDepthTextureIndex;
    uint DepthStencilTextureIndex;
    uint __Pad0;
    // 16 byte boundary
};

struct GBufferTexturePack
{
    Texture2D AlbedoMetalness;
    Texture2D Roughness;
    Texture2D<uint4> Normal;
    Texture2D<uint4> Motion;
    Texture2D<uint4> TypeAndMaterialIndex;
    Texture2D ViewDepth;
    Texture2D DepthStencil;
};

struct GBufferPixelOut
{
    float4 AlbedoMetalness : SV_Target0;
    float Roughness : SV_Target1;
    uint Normal : SV_Target2;
    uint Motion : SV_Target3;
    uint TypeAndMaterialIndex : SV_Target4;
    float ViewDepth : SV_Target5;
};

struct GBufferStandard
{
    float3 Albedo;
    float3 Normal;
    float3 Motion;
    float Roughness;
    float Metalness;
    uint MaterialIndex;
};

struct GBufferEmissive
{
    uint LightIndex;
    float3 Motion;
};

GBufferPixelOut GetStandardGBufferPixelOutput(float3 albedo, float metalness, float roughness, float3 normal, float3 motion, uint materialIndex, float viewDepth)
{
    GBufferPixelOut output;
    output.AlbedoMetalness = float4(albedo, metalness);
    output.Roughness = roughness;
    output.Normal = EncodeNormalSignedOct(normal);
    output.Motion = EncodeNormalSignedOct(motion);
    output.TypeAndMaterialIndex = (GBufferTypeStandard << 4) | (materialIndex & 0x000000F);
    output.ViewDepth = viewDepth;
    return output;
}

GBufferPixelOut GetEmissiveGBufferPixelOutput(uint lightIndex, float3 motion, float viewDepth)
{
    GBufferPixelOut output;
    output.AlbedoMetalness = 0.0;
    output.Roughness = 0.0;
    output.Normal = lightIndex;
    output.Motion = EncodeNormalSignedOct(motion);
    output.TypeAndMaterialIndex = GBufferTypeEmissive << 4;
    output.ViewDepth = viewDepth;
    return output;
}

uint LoadGBufferType(GBufferTexturePack textures, uint2 texelIndex)
{
    uint type = textures.TypeAndMaterialIndex.Load(uint3(texelIndex, 0)).x;
    return type >> 4;
}

void LoadStandardGBuffer(inout GBufferStandard gBuffer, GBufferTexturePack textures, uint2 texelIndex)
{
    uint3 loadIndex = uint3(texelIndex, 0);

    float4 albedoMetalness = textures.AlbedoMetalness.Load(loadIndex).xyzw;
    float roughness = textures.Roughness.Load(loadIndex).r;
    float3 normal = DecodeNormalSignedOct(textures.Normal.Load(loadIndex).x);
    float3 motion = DecodeNormalSignedOct(textures.Motion.Load(loadIndex).x);
    uint typeAndMaterialIndex = textures.TypeAndMaterialIndex.Load(loadIndex).x;
    uint materialIndex = typeAndMaterialIndex & 0x0000000F;

    gBuffer.Albedo = albedoMetalness.rgb;
    gBuffer.Metalness = albedoMetalness.a;
    gBuffer.Roughness = roughness;
    gBuffer.Normal = normal;
    gBuffer.Motion = motion;
    gBuffer.MaterialIndex = materialIndex;
}

void LoadEmissiveGBuffer(inout GBufferEmissive gBuffer, GBufferTexturePack textures, uint2 texelIndex)
{
    uint3 loadIndex = uint3(texelIndex, 0);

    uint lightIndex = textures.Normal.Load(loadIndex).x;
    float3 motion = DecodeNormalSignedOct(textures.Motion.Load(loadIndex).x);

    gBuffer.LightIndex = lightIndex;
    gBuffer.Motion = motion;
}

float3 LoadGBufferNormal(Texture2D<uint4> normals, uint2 texelIndex)
{
    return DecodeNormalSignedOct(normals.Load(uint3(texelIndex, 0)).x);
}

float LoadGBufferRoughness(Texture2D roughness, uint2 texelIndex)
{
    return roughness.Load(uint3(texelIndex, 0)).r;
}

#endif