static const float FloatMax = 3.402823466e+38;

static const float3x3 Matrix3x3Identity = 
{
    float3(1, 0, 0),
    float3(0, 1, 0),
    float3(0, 0, 1)
};

static const float4x4 Matrix4x4Identity =
{
    float4(1, 0, 0, 0),
    float4(0, 1, 0, 0),
    float4(0, 0, 1, 0),
    float4(0, 0, 0, 1)
};

float3x3 Matrix3x3ColumnMajor(float3 column0, float3 column1, float3 column2)
{
    float3x3 M;
    M[0] = float3(column0.x, column1.x, column2.x);
    M[1] = float3(column0.y, column1.y, column2.y);
    M[2] = float3(column0.z, column1.z, column2.z);
    return M;
}

float4x4 Matrix4x4ColumnMajor(float4 column0, float4 column1, float4 column2, float4 column3)
{
    float4x4 M;
    M[0] = float4(column0.x, column1.x, column2.x, column3.x);
    M[1] = float4(column0.y, column1.y, column2.y, column3.y);
    M[2] = float4(column0.z, column1.z, column2.z, column3.z);
    M[3] = float4(column0.w, column1.w, column2.w, column3.w);
    return M;
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

float2 TexelIndexToUV(uint2 index, uint2 textureSize)
{
    return float2(index) / textureSize;
}

float3 VoxelIndexToUVW(uint3 index, uint3 textureSize)
{
    return float3(index) / textureSize;
}

uint2 UVToTexelIndex(float2 uv, uint2 textureSize)
{
    return clamp(uv * textureSize, 0.xx, textureSize - 1);
}

uint3 UVWToVoxelIndex(float3 uvw, uint3 textureSize)
{
    return clamp(uvw * textureSize, 0.xxx, textureSize - 1);
}
