#ifndef _Utils__
#define _Utils__

#include "Constants.hlsl"

float Square(float v)
{
    return v * v;
}

float Max(float2 v)
{
    return max(v.x, v.y);
}

float Max(float3 v)
{
    return max(v.x, max(v.y, v.z));
}

float Max(float4 v)
{
    return max(max(v.x, max(v.y, v.z)), v.w);
}

float Flatten3DIndexFloat(float3 index3D, float3 dimensions)
{
    return (index3D.x) + (index3D.y * dimensions.x) + (index3D.z * dimensions.x * dimensions.y);
}

int Flatten3DIndexInt(int3 index3D, int3 dimensions)
{
    return (index3D.x) + (index3D.y * dimensions.x) + (index3D.z * dimensions.x * dimensions.y);
}

uint VectorOctant(float3 normalizedVector)
{
    uint index = 0;

    if (abs(normalizedVector.x) > abs(normalizedVector.z))
    {
        index = normalizedVector.x < 0 ? 0 : 2;
    }
    else {
        index = normalizedVector.z < 0 ? 3 : 1;
    }

    if (normalizedVector.y < 0)
    {
        index += 4;
    }

    return index;
}

float2 CountFittingTexels(float2 originalTextureResolution, float2 otherTextureResolution)
{
    return floor(originalTextureResolution / otherTextureResolution); 
}

// Composes a floating point value with the magnitude of x and the sign of y
//
float CopySign(float x, float y)
{
    if ((x < 0 && y > 0) || (x > 0 && y < 0))
    {
        return -x;
    }
        
    return x;
}

float Sign(float x)
{
    return x < 0.0 ? -1.0 : 1.0;
}

float2 Refit0to1ValuesToTexelCenter(float2 values, float2 textureSize)
{
    float2 scale = (textureSize - 1.0) / textureSize;
    float2 bias = 0.5 / textureSize;
    return values * scale + bias;
}

float2 TexelIndexToUV(uint2 index, uint2 textureSize)
{
    return float2(index + 0.5) / textureSize;
}

float3 VoxelIndexToUVW(uint3 index, uint3 textureSize)
{
    return float3(index + 0.5) / textureSize;
}

uint2 UVToTexelIndex(float2 uv, uint2 textureSize)
{
    return clamp(uv * textureSize, 0.xx, textureSize - 1);
}

uint3 UVWToVoxelIndex(float3 uvw, uint3 textureSize)
{
    return clamp(uvw * textureSize, 0.xxx, textureSize - 1);
}

float2 NDCToUV(float3 ndcPoint)
{
    float2 uv = (ndcPoint.xy + 1.0) * 0.5; // [-1; 1] to [0; 1]
    uv.y = 1.0 - uv.y; // Conform DX specs
    return uv;
}

#endif