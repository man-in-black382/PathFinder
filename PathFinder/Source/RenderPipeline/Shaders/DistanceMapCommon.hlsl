#ifndef _DistanceMapCommon__
#define _DistanceMapCommon__

struct PassData
{
    uint4 DisplacementMapSize;
    uint4 DistanceAtlasIndirectionMapSize;
    uint DisplacementMapSRVIndex;
    uint DistanceAltasIndirectionMapUAVIndex;
    uint DistanceAltasUAVIndex;
    uint FloodStep;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "ColorConversion.hlsl"
#include "Utils.hlsl"

// Holds positions of and distances to voxels in 8 directions
struct DistanceFieldCones
{
    float4 PositionsAndDistances[8];
};

RWStructuredBuffer<DistanceFieldCones> ReadOnlyConesBuffer : register(u0);
RWStructuredBuffer<DistanceFieldCones> WriteOnlyConesBuffer : register(u1);

static const int VoxelUnderDisplacementMap = -2.0;
static const int VoxelFree = -1.0;
static const int VoxelIntersectedByDisplacementSurface = 0.0;

struct VoxelIntersectionInfo
{
    bool VoxelIntersectsDisplacementMap;
    bool VoxelIsUnderDisplacementMap;
};

float VoxelCentersDistance(uint3 currentVoxel, uint3 neighbourVoxel, uint3 voxelGridSize)
{
    float3 voxelGridSizeInv = 1.0f / float3(voxelGridSize);
    float3 voxelNormHalfSize = voxelGridSizeInv * 0.5;

    float3 currentVoxelCenterInTexSpace = (float3(currentVoxel) * voxelGridSizeInv) + voxelNormHalfSize;
    float3 neighbourVoxeCenterlInTexSpace = (float3(neighbourVoxel) * voxelGridSizeInv) + voxelNormHalfSize;

    return distance(neighbourVoxeCenterlInTexSpace, currentVoxelCenterInTexSpace);
}

VoxelIntersectionInfo VoxelIntersectsDisplacementMap(Texture2D displacementMap, uint3 voxel)
{
    float3 voxelNormSize = 1.0f / float3(PassDataCB.DistanceAtlasIndirectionMapSize.xyz);
    float3 currentVoxelInTexSpace = VoxelIndexToUVW(voxel, PassDataCB.DistanceAtlasIndirectionMapSize.xyz);

    uint2 samplingOrigin = currentVoxelInTexSpace.xy * PassDataCB.DisplacementMapSize.xy;
    uint2 texelCount = voxelNormSize.xy * PassDataCB.DisplacementMapSize.xy;

    VoxelIntersectionInfo intersectionInfo;
    intersectionInfo.VoxelIntersectsDisplacementMap = false;
    intersectionInfo.VoxelIsUnderDisplacementMap = true;

    float voxelBottom = currentVoxelInTexSpace.z;
    float voxelTop = currentVoxelInTexSpace.z + voxelNormSize.z;

    uint xBoundary = samplingOrigin.x + texelCount.x;
    uint yBoundary = samplingOrigin.y + texelCount.y;

    for (uint x = samplingOrigin.x; x < xBoundary; ++x)
    {
        for (uint y = samplingOrigin.y; y < yBoundary; ++y)
        {
            float displacement = displacementMap.Load(uint3(x, y, 0)).r;

            if (displacement >= voxelBottom && displacement < voxelTop)
            {
                intersectionInfo.VoxelIntersectsDisplacementMap = true;
                intersectionInfo.VoxelIsUnderDisplacementMap = false;
                return intersectionInfo;
            }
            else if (voxelTop >= displacement)
            {
                intersectionInfo.VoxelIsUnderDisplacementMap = false;
            }
        }
    }

    return intersectionInfo;
}

#endif