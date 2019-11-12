struct PassData
{
    uint DisplacementMapSRVIndex;
    uint DistanceMapUAVIndex;
};

#define PassDataType PassData

#include "BaseRootSignature.hlsl"
#include "ColorConversion.hlsl"

[numthreads(16, 16, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D displacementMap = Textures2D[PassDataCB.DisplacementMapSRVIndex];
    RWTexture3D<float4> distanceMap = RW_Float4_Textures3D[PassDataCB.DistanceMapUAVIndex];

    uint3 loadCoords = uint3(dispatchThreadID.xy, 0);
}