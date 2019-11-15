struct PassData
{
    uint DisplacementMapSRVIndex;
    uint DistanceAltasIndirectionMapUAVIndex;
    uint DistanceAltasUAVIndex;
};

#define PassDataType PassData

#include "BaseRootSignature.hlsl"
#include "ColorConversion.hlsl"

[numthreads(8, 8, 8)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D displacementMap = Textures2D[PassDataCB.DisplacementMapSRVIndex];
    RWTexture2D<float4> distanceAtlasIndirectionMap = RW_Float4_Textures2D[PassDataCB.DistanceAltasIndirectionMapUAVIndex];

    uint width;
    uint height;

    displacementMap.GetDimensions(width, height);

    float d = 0.0;

    for (uint x = 0; x < width; ++x)
    {
        for (uint y = 0; y < height; ++y)
        {
            float displcement = displacementMap.Load(uint3(x, y, 0)).r;

            d = displcement;
        }
    }

    //uint3 loadCoords = uint3(dispatchThreadID.xy, 0);
    distanceAtlasIndirectionMap[dispatchThreadID.xy] = float4(d, 0.0, 0.0, 0.0);
}