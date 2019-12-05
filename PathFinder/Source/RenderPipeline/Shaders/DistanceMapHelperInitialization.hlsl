#include "DistanceMapCommon.hlsl"

[numthreads(8, 8, 8)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D displacementMap = Textures2D[PassDataCB.DisplacementMapSRVIndex];

    VoxelIntersectionInfo intersectionInfo = VoxelIntersectsDisplacementMap(displacementMap, dispatchThreadID);

    int bufferIndex = Flatten3DIndexInt(dispatchThreadID, PassDataCB.DistanceAtlasIndirectionMapSize.xyz);

    float4 output;

    if (intersectionInfo.VoxelIntersectsDisplacementMap)
    {
        output = float4(dispatchThreadID, VoxelIntersectedByDisplacementSurface);
    }
    else if (intersectionInfo.VoxelIsUnderDisplacementMap)
    {
        output = float4(VoxelUnderDisplacementMap, VoxelUnderDisplacementMap, VoxelUnderDisplacementMap, VoxelUnderDisplacementMap);
    }
    else 
    {
        output = float4(VoxelFree, VoxelFree, VoxelFree, VoxelFree);
    }

    DistanceFieldCones cones;

    for (uint i = 0; i < 8; ++i)
    {
        cones.PositionsAndDistances[i] = output;
    }

    ReadOnlyConesBuffer[bufferIndex] = cones;
    WriteOnlyConesBuffer[bufferIndex] = cones;
}