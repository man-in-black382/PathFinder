#ifndef _UAVClear__
#define _UAVClear__

static const uint ClearOpTextureFloat = 0;
static const uint ClearOpTextureUInt = 1;
static const uint ClearOpBufferFloat = 2;
static const uint ClearOpBufferUInt = 3;

struct PassData
{
    float4 FloatValues;
    uint4 UIntValues;
    uint Operation;
    uint OutputTexIdx;
    uint BufferSize;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

RWStructuredBuffer<float> FloatBuffer : register(u0);
RWStructuredBuffer<uint> UIntBuffer : register(u1);

static const int GroupDimensionSize = 256;

[numthreads(GroupDimensionSize, 1, 1)]
void CSMain(int3 DTid : SV_DispatchThreadID)
{
    if (PassDataCB.Operation == ClearOpTextureFloat)
    {
        RWTexture2D<float4> outputTexture = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];
        outputTexture[DTid.xy] = PassDataCB.FloatValues;
    }
    else if (PassDataCB.Operation == ClearOpTextureUInt)
    {
        RWTexture2D<uint4> outputTexture = RW_UInt4_Textures2D[PassDataCB.OutputTexIdx];
        outputTexture[DTid.xy] = PassDataCB.UIntValues;
    }
    else if (PassDataCB.Operation == ClearOpBufferFloat)
    {
        if (DTid.x >= PassDataCB.BufferSize)
            return;

        FloatBuffer[DTid.x] = PassDataCB.FloatValues.x;
    }
    else if (PassDataCB.Operation == ClearOpBufferUInt)
    {
        if (DTid.x >= PassDataCB.BufferSize)
            return;

        UIntBuffer[DTid.x] = PassDataCB.UIntValues.x;
    }
}

#endif