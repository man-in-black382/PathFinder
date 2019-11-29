#include "DistanceMapCommon.hlsl"

[numthreads(8, 8, 8)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D displacementMap = Textures2D[PassDataCB.DisplacementMapSRVIndex];
    RWTexture3D<float4> JFAHelperTexture0 = RW_Float4_Textures3D[PassDataCB.ReadOnlyJFAConesIndirectionUAVIndex];
    RWTexture3D<float4> JFAHelperTexture1 = RW_Float4_Textures3D[PassDataCB.WriteOnlyJFAConesIndirectionUAVIndex];

    VoxelIntersectionInfo intersectionInfo = VoxelIntersectsDisplacementMap(displacementMap, dispatchThreadID);

    float4 output;

    if (intersectionInfo.VoxelIntersectsDisplacementMap)
    {
        output = float4(dispatchThreadID, VoxelOccupied);
    }
    else if (intersectionInfo.VoxelIsUnderDisplacementMap)
    {
        output = float4(VoxelUnderDisplacementMap, VoxelUnderDisplacementMap, VoxelUnderDisplacementMap, VoxelUnderDisplacementMap);
    }
    else 
    {
        output = float4(VoxelFree, VoxelFree, VoxelFree, VoxelFree);
    }

    JFAHelperTexture0[dispatchThreadID] = output;
    JFAHelperTexture1[dispatchThreadID] = output;
}