struct PassData
{
    uint4 DisplacementMapSize;
    uint4 DistanceAtlasIndirectionMapSize;
    uint DisplacementMapSRVIndex;
    uint DistanceAltasIndirectionMapUAVIndex;
    uint DistanceAltasUAVIndex;
    uint ReadOnlyJFAHelperUAVIndex;
    uint WriteOnlyJFAHelperUAVIndex;
    uint FloodStep;
};

#define PassDataType PassData

#include "BaseRootSignature.hlsl"
#include "ColorConversion.hlsl"

static const int VoxelUnderDisplacementMap = -2.0;
static const int VoxelFree = -1.0;
static const int VoxelOccupied = 0.0;

struct VoxelIntersectionInfo
{
    bool VoxelIntersectsDisplacementMap;
    bool VoxelIsUnderDisplacementMap;
};

float FindClosestDisplacementMapPointDistance(Texture2D displacementMap, uint3 currentVoxel, uint3 neighbourVoxel)
{
    uint2 displacementMapSize = PassDataCB.DisplacementMapSize.xy;
    uint3 vogelGridSize = PassDataCB.DistanceAtlasIndirectionMapSize.xyz;

    float3 voxelNormSize = 1.0f / vogelGridSize;
    float3 voxelNormHalfSize = voxelNormSize * 0.5;

    float3 currentVoxelCenterInTexSpace = (float3(currentVoxel) / vogelGridSize) + voxelNormHalfSize;
    float3 neighbourVoxelInTexSpace = float3(neighbourVoxel) / vogelGridSize;

    uint2 samplingOrigin = neighbourVoxelInTexSpace.xy * displacementMapSize;
    uint2 texelCount = voxelNormSize.xy * displacementMapSize;

    float minDistance = 3.402823466e+38F;

    for (uint x = samplingOrigin.x; x < samplingOrigin.x + texelCount.x; ++x)
    {
        for (uint y = samplingOrigin.y; y < samplingOrigin.y + texelCount.y; ++y)
        {
            float displacement = displacementMap.Load(uint3(x, y, 0)).r;

            float2 uv = float2(x / float(displacementMapSize.x), y / float(displacementMapSize.y));
            float3 sample3DPosition = float3(uv, displacement);
            float currentDistance = distance(sample3DPosition, currentVoxelCenterInTexSpace);
            minDistance = min(minDistance, currentDistance);
        }
    }

    return minDistance;
}

VoxelIntersectionInfo VoxelIntersectsDisplacementMap(Texture2D displacementMap, uint3 voxel)
{
    float3 voxelNormSize = 1.0f / float3(PassDataCB.DistanceAtlasIndirectionMapSize.xyz);
    float3 currentVoxelInTexSpace = (float3(voxel) / PassDataCB.DistanceAtlasIndirectionMapSize.xyz);

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