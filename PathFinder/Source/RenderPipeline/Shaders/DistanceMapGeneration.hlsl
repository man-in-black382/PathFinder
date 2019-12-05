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
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D displacementMap = Textures2D[PassDataCB.DisplacementMapSRVIndex];

    if (dispatchThreadID.x > 127 || dispatchThreadID.y > 127 || dispatchThreadID.z > 63) return;

    int dispatchThreadIDFlat = Flatten3DIndexInt(dispatchThreadID, PassDataCB.DistanceAtlasIndirectionMapSize.xyz);

    DistanceFieldCones cones = ReadOnlyConesBuffer[dispatchThreadIDFlat];

    // Make sure that the other buffer is updated
    WriteOnlyConesBuffer[dispatchThreadIDFlat] = cones;

    float3 currentClosestVoxel = cones.PositionsAndDistances[0].xyz;
    float currentClosestDistance = cones.PositionsAndDistances[0].w;

    // This voxel is intersected by displacement map (a 'Seed' in terminology of Jump Flooding algorithm)
    bool isOriginalSeed = all(int3(currentClosestVoxel) == dispatchThreadID);

    // This voxel is located under all of the displacement values located in the corresponding UV rect of displacement map
    bool isCompletelyUnderDisplacementSurface = int(currentClosestDistance) == VoxelUnderDisplacementMap;

    if (isOriginalSeed || isCompletelyUnderDisplacementSurface)
    {
        return;
    }

    for (uint i = 0; i < 26; ++i)
    {
        int3 neighbourVoxel = dispatchThreadID + Offsets[i] * PassDataCB.FloodStep;
        
        int coneIndex = 0;// VectorConeIndex(normalize(float3(Offsets[i])));

        // If the neighbor is outside the bounds, skip it
        if (any(neighbourVoxel < 0) || any(neighbourVoxel >= PassDataCB.DistanceAtlasIndirectionMapSize.xyz))
        {
            continue;
        }

        int neighborVoxelFlat = Flatten3DIndexInt(neighbourVoxel, PassDataCB.DistanceAtlasIndirectionMapSize.xyz);

        float3 closestVoxelOfNeighbour = ReadOnlyConesBuffer[neighborVoxelFlat].PositionsAndDistances[coneIndex].xyz;

        bool neighbourHasClosestVoxelRecorded = all(closestVoxelOfNeighbour >= 0.0f);

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
            currentClosestDistance = neighbourClosestDistance;
            WriteOnlyConesBuffer[dispatchThreadIDFlat].PositionsAndDistances[coneIndex] = float4(closestVoxelOfNeighbour, neighbourClosestDistance);
        }
    }
}