#ifndef _UAVClear__
#define _UAVClear__

static const uint TextureFormatFloat = 0;
static const uint TextureFormatUInt = 1;

struct PassData
{
    float4 FloatValues;
    uint4 UIntValues;
    uint TextureType;
    uint OutputTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 DTid : SV_DispatchThreadID, int3 GTid : SV_GroupThreadID)
{
    if (PassDataCB.TextureType == TextureFormatFloat)
    {
        RWTexture2D<float4> outputTexture = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];
        outputTexture[DTid.xy] = PassDataCB.FloatValues;
    }
    else if (PassDataCB.TextureType == TextureFormatUInt)
    {
        RWTexture2D<uint4> outputTexture = RW_UInt4_Textures2D[PassDataCB.OutputTexIdx];
        outputTexture[DTid.xy] = PassDataCB.UIntValues;
    }
}

#endif