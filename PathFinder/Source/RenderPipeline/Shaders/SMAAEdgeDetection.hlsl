#ifndef _SMAAEdgeDetection__
#define _SMAAEdgeDetection__

struct PassData
{
    uint InputTexIdx;
};

#define PassDataType PassData

#include "SMAACommon.hlsl"
#include "SMAA.hlsl"

struct VertexOut
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float4 Offset0 : OFFSET_1;
    float4 Offset1 : OFFSET_2;
    float4 Offset2 : OFFSET_3;
};


VertexOut VSMain(uint indexId : SV_VertexID)
{
    float4 position = float4(Vertices[indexId], 0.0, 1.0);
    float2 uv = UVs[indexId];
    uv.y = 1.0 - uv.y;

    float4 offsets[3];
    SMAAEdgeDetectionVS(uv, offsets);

    VertexOut vout;
    vout.Position = position;
    vout.UV = uv;
    vout.Offset0 = offsets[0];
    vout.Offset1 = offsets[1];
    vout.Offset2 = offsets[2];

    return vout;
}

struct PixelOut
{
    float2 Edges : SV_Target0;
};

PixelOut PSMain(VertexOut pin)
{
    float4 offsets[3] = { pin.Offset0, pin.Offset1, pin.Offset2 };
    Texture2D image = Textures2D[PassDataCB.InputTexIdx];

    PixelOut pixelOut;
    pixelOut.Edges = SMAAColorEdgeDetectionPS(pin.UV, offsets, image);
    return pixelOut;
}

#endif