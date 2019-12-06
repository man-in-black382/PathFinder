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

// Checks whether voxel is intersected by displacement map 
// (a 'Seed' in terminology of Jump Flooding algorithm)
bool IsOriginalSeed(DistanceFieldCones cones, int3 currentVoxel)
{
    return all(int3(cones.PositionsAndDistances[0].xyz) == currentVoxel) &&
        all(int3(cones.PositionsAndDistances[1].xyz) == currentVoxel) &&
        all(int3(cones.PositionsAndDistances[2].xyz) == currentVoxel) &&
        all(int3(cones.PositionsAndDistances[3].xyz) == currentVoxel) &&
        all(int3(cones.PositionsAndDistances[4].xyz) == currentVoxel) &&
        all(int3(cones.PositionsAndDistances[5].xyz) == currentVoxel) &&
        all(int3(cones.PositionsAndDistances[6].xyz) == currentVoxel) &&
        all(int3(cones.PositionsAndDistances[7].xyz) == currentVoxel);
}

// Checks whether voxel is located under all of the displacement values
// located in the corresponding UV rect of displacement map
bool IsUnderDisplacementSurface(DistanceFieldCones cones)
{
    return int(cones.PositionsAndDistances[0].w) == VoxelUnderDisplacementMap &&
        int(cones.PositionsAndDistances[1].w) == VoxelUnderDisplacementMap &&
        int(cones.PositionsAndDistances[2].w) == VoxelUnderDisplacementMap &&
        int(cones.PositionsAndDistances[3].w) == VoxelUnderDisplacementMap &&
        int(cones.PositionsAndDistances[4].w) == VoxelUnderDisplacementMap &&
        int(cones.PositionsAndDistances[5].w) == VoxelUnderDisplacementMap &&
        int(cones.PositionsAndDistances[6].w) == VoxelUnderDisplacementMap &&
        int(cones.PositionsAndDistances[7].w) == VoxelUnderDisplacementMap;
}

[numthreads(8, 8, 8)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D displacementMap = Textures2D[PassDataCB.DisplacementMapSRVIndex];

    uint3 voxelGridSize = PassDataCB.DistanceAtlasIndirectionMapSize.xyz;

    if (any(dispatchThreadID >= voxelGridSize))
    {
        return;
    }

    int currentVoxelFlat = Flatten3DIndexInt(dispatchThreadID, voxelGridSize);

    DistanceFieldCones cones = ReadOnlyConesBuffer[currentVoxelFlat];

    // Make sure that the other buffer is always updated
    WriteOnlyConesBuffer[currentVoxelFlat] = cones;

    float currentClosestDistances[8] = { 
        cones.PositionsAndDistances[0].w,
        cones.PositionsAndDistances[1].w,
        cones.PositionsAndDistances[2].w,
        cones.PositionsAndDistances[3].w,
        cones.PositionsAndDistances[4].w,
        cones.PositionsAndDistances[5].w,
        cones.PositionsAndDistances[6].w,
        cones.PositionsAndDistances[7].w
    };

    if (IsOriginalSeed(cones, dispatchThreadID) || IsUnderDisplacementSurface(cones))
    {
        return;
    }

    for (uint i = 0; i < 26; ++i)
    {
        int3 neighbourVoxel = dispatchThreadID + Offsets[i] * PassDataCB.FloodStep;
        
        int coneIndex = VectorOctant(normalize(float3(Offsets[i])));
        bool isOutOfBounds = any(neighbourVoxel < 0) || any(neighbourVoxel >= voxelGridSize);

        if (isOutOfBounds)
        {
            continue;
        }

        int neighborVoxelFlat = Flatten3DIndexInt(neighbourVoxel, voxelGridSize);
        float3 closestVoxelOfNeighbour = ReadOnlyConesBuffer[neighborVoxelFlat].PositionsAndDistances[coneIndex].xyz;
        bool neighbourHasClosestVoxelRecorded = all(closestVoxelOfNeighbour >= 0);

        if (!neighbourHasClosestVoxelRecorded)
        {
            continue;
        }

        float neighbourClosestDistance = VoxelCentersDistance(dispatchThreadID, closestVoxelOfNeighbour, voxelGridSize);

        // If this voxel doesn't contain info of some other seed that is closest to it,
        // then we treat closest neighbor voxel as a closest seed and store it's data in this voxel
        bool noClosestSeedInfoPresent = int(currentClosestDistances[coneIndex]) == VoxelFree;

        if (noClosestSeedInfoPresent || neighbourClosestDistance < currentClosestDistances[coneIndex])
        {
            currentClosestDistances[coneIndex] = neighbourClosestDistance;
            WriteOnlyConesBuffer[currentVoxelFlat].PositionsAndDistances[coneIndex] = float4(closestVoxelOfNeighbour, neighbourClosestDistance);
        }
    }
}