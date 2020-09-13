#ifndef _SMAANeighborhoodBlending__
#define _SMAANeighborhoodBlending__

struct PassData
{
    uint InputImageTexIdx;
    uint BlendingWeightsTexIdx;
};

#define PassDataType PassData

#include "SMAACommon.hlsl"
#include "SMAA.hlsl"

struct VertexOut
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float4 Offset : OFFSET;
};

VertexOut VSMain(uint indexId : SV_VertexID)
{
    float4 position = float4(Vertices[indexId], 0.0, 1.0);
    float2 uv = UVs[indexId];
    uv.y = 1.0 - uv.y;

    float4 offset;
    SMAANeighborhoodBlendingVS(uv, offset);

    VertexOut vout;
    vout.Position = position;
    vout.UV = uv;
    vout.Offset = offset;

    return vout;
}

struct PixelOut
{
    float4 Color : SV_Target0;
};

PixelOut PSMain(VertexOut pin)
{
    Texture2D image = Textures2D[PassDataCB.InputImageTexIdx];
    Texture2D blendingWeights = Textures2D[PassDataCB.BlendingWeightsTexIdx];

    PixelOut pixelOut;
    pixelOut.Color = SMAANeighborhoodBlendingPS(pin.UV, pin.Offset, image, blendingWeights);
    return pixelOut;
}

#endif