struct BlurPassData
{
    uint BlurRadius;
    float Weights[64];
    uint InputTextureIndex;
    uint OutputTextureIndex;
};

#define PassDataType BlurPassData

#include "BaseRootSignature.hlsl"

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-attributes-numthreads

static const uint GroupSize = 256;

groupshared float3 gCache[GroupSize];

[numthreads(GroupSize, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID)
{
    Texture2D source = Textures2D[PassCBData.InputTextureIndex];
    RWTexture2D destination = RWTextures2D[PassCBData.OutputTextureIndex];

    uint2 boundaries = GlobalDataCB.PipelineRTResolution;

    

    if (isInPositiveHalf)
    {

    }
}