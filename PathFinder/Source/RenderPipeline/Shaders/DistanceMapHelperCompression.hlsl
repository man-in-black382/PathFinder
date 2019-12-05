#include "DistanceMapCommon.hlsl"
#include "Packing.hlsl"

static const float DistanceFieldMaxVoxelDistance = sqrt(3);

uint4 PackConeData(DistanceFieldCones cones)
{
    return uint4(
        PackUnorm2x16(max(cones.PositionsAndDistances[0].w, 0.0), max(cones.PositionsAndDistances[1].w, 0.0), DistanceFieldMaxVoxelDistance),
        PackUnorm2x16(max(cones.PositionsAndDistances[2].w, 0.0), max(cones.PositionsAndDistances[3].w, 0.0), DistanceFieldMaxVoxelDistance),
        PackUnorm2x16(max(cones.PositionsAndDistances[4].w, 0.0), max(cones.PositionsAndDistances[5].w, 0.0), DistanceFieldMaxVoxelDistance),
        PackUnorm2x16(max(cones.PositionsAndDistances[6].w, 0.0), max(cones.PositionsAndDistances[7].w, 0.0), DistanceFieldMaxVoxelDistance)
    );
}

[numthreads(8, 8, 8)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture3D<float4> distanceAtlasIndirectionMap = RW_Float4_Textures3D[PassDataCB.DistanceAltasIndirectionMapUAVIndex];
    RWTexture3D<uint4> distanceAtlas = RW_UInt4_Textures3D[PassDataCB.DistanceAltasUAVIndex];

    int flatIndex = Flatten3DIndexInt(dispatchThreadID, PassDataCB.DistanceAtlasIndirectionMapSize.xyz);

    DistanceFieldCones cones = ReadOnlyConesBuffer[flatIndex];
    distanceAtlas[dispatchThreadID] = PackConeData(cones);
}