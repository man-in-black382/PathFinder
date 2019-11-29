#include "DistanceMapCommon.hlsl"

[numthreads(8, 8, 8)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture3D<float4> JFAHelperTexture = RW_Float4_Textures3D[PassDataCB.ReadOnlyJFAConesIndirectionUAVIndex];


    RWTexture3D<float4> distanceAtlasIndirectionMap = RW_Float4_Textures3D[PassDataCB.DistanceAltasIndirectionMapUAVIndex];
    RWTexture3D<float4> distanceAtlas = RW_Float4_Textures3D[PassDataCB.DistanceAltasUAVIndex];

    distanceAtlasIndirectionMap[dispatchThreadID].r = JFAHelperTexture[dispatchThreadID].w;
}