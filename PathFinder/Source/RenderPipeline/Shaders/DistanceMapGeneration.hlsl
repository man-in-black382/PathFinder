#include "DistanceMapCommon.hlsl"

// Jump Flooding Algorithm

static const int3 Offsets[26] = {
    int3(0,0,1), int3(1,0,1), int3(-1,0,1),
    int3(0,1,1) , int3(0,-1,1), int3(1,1,1),
    int3(1,-1,1), int3(-1,1,1), int3(-1,-1,1),
    int3(1,0,0), int3(-1,0,0), int3(0,1,0),
    int3(0,-1,0), int3(1,1,0), int3(1,-1,0),
    int3(-1,1,0), int3(-1,-1,0), int3(0,0,-1),
    int3(1,0,-1), int3(-1,0,-1) , int3(0,1,-1),
    int3(0,-1,-1), int3(1,1,-1),
    int3(1,-1,-1), int3(-1,1,-1), int3(-1,-1,-1)
};

[numthreads(8, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D displacementMap = Textures2D[PassDataCB.DisplacementMapSRVIndex];
    RWTexture3D<float4> ReadJFAHelperTexture = RW_Float4_Textures3D[PassDataCB.ReadOnlyJFAHelperUAVIndex];
    RWTexture3D<float4> WriteJFAHelperTexture = RW_Float4_Textures3D[PassDataCB.WriteOnlyJFAHelperUAVIndex];

    uint3 currentClosestVoxel = ReadJFAHelperTexture[dispatchThreadID].xyz;
    float currentClosestDistance = ReadJFAHelperTexture[dispatchThreadID].w;

    // This voxel is intersected by displacement map (a seed in terminology of Jump Flooding algorithm)
    bool isOriginalSeed = all(int3(currentClosestVoxel) == dispatchThreadID);

    // This voxel is located under all of the displacement values located in the corresponding UV rect of displacement map
    bool isCompletelyUnderDisplacementSurface = int(currentClosestDistance) == VoxelUnderDisplacementMap;

    if (isOriginalSeed || isCompletelyUnderDisplacementSurface)
    {
        return;
    }

    for (uint i = 0; i < 26; ++i)
    {
        int3 neighbourVoxel = dispatchThreadID; 
        neighbourVoxel += Offsets[i] * PassDataCB.FloodStep;
        
        // If the neighbor is outside the bounds, skip it
        if (any(neighbourVoxel < 0) || any(neighbourVoxel >= PassDataCB.DistanceAtlasIndirectionMapSize.xyz))
        {
            continue;
        }

        float3 closestVoxelOfNeighbour = ReadJFAHelperTexture[neighbourVoxel].xyz;
        bool neighbourHasClosestVoxelRecorded = all(neighbourVoxel >= 0);

        if (!neighbourHasClosestVoxelRecorded)
        {
            continue;
        }

        float neighbourClosestDistance = FindClosestDisplacementMapPointDistance(displacementMap, dispatchThreadID, closestVoxelOfNeighbour);

        // If this voxel doesn't contain info of some other seed that is closest to it,
        // then we treat closest neighbor voxel as a closest seed and store it's data in this voxel
        bool noClosestSeedInfoPresent = int(currentClosestDistance) == VoxelFree;

        if (noClosestSeedInfoPresent || neighbourClosestDistance < currentClosestDistance)
        {
            WriteJFAHelperTexture[dispatchThreadID] = float4(closestVoxelOfNeighbour, neighbourClosestDistance);
            currentClosestDistance = neighbourClosestDistance;
        }
    }
}