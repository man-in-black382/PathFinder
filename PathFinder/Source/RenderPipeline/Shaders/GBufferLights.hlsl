#ifndef _GBufferLights__
#define _GBufferLights__

#include "MandatoryEntryPointInclude.hlsl"
#include "GBuffer.hlsl"
#include "Vertices.hlsl"
#include "Geometry.hlsl"
#include "Matrix.hlsl"
#include "Light.hlsl"

struct RootConstants
{
    uint LightTableIndex;
};

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);
StructuredBuffer<Light> LightTable : register(t0);
StructuredBuffer<Vertex1P1N1UV1T1BT> UnifiedVertexBuffer : register(t1);
StructuredBuffer<IndexU32> UnifiedIndexBuffer : register(t2);

//------------------------  Vertex  ------------------------------//

struct VertexOut
{
    float4 Position : SV_POSITION;
    float3 LightOrientation : NORMAL;
    float ViewDepth : VIEW_DEPTH;
    float2 LocalSpacePosition : DIST_FROM_CENTER;
    float2 LightSize : LIGHT_SIZE;
};

VertexOut VSMain(uint vertexId : SV_VertexID)
{
    Light light = LightTable[RootConstantBuffer.LightTableIndex];

    // Load index and vertex
    IndexU32 index = UnifiedIndexBuffer[light.UnifiedIndexBufferOffset + vertexId];
    Vertex1P1N1UV1T1BT vertex = UnifiedVertexBuffer[light.UnifiedVertexBufferOffset + index.Index];

    float2 localSpacePosition = vertex.Position.xy;

    float4 WSPosition = mul(light.ModelMatrix, vertex.Position);
    float4 CSPosition = mul(FrameDataCB.CurrentFrameCamera.View, WSPosition);
    float4 ClipSPosition = mul(FrameDataCB.CurrentFrameCamera.Projection, CSPosition);

    VertexOut vout;
    vout.Position = ClipSPosition;
    vout.LightOrientation = light.Orientation.xyz;
    vout.ViewDepth = CSPosition.z;
    vout.LocalSpacePosition = light.LightType != LightTypeRectangle ? localSpacePosition : 0.xx;
    vout.LightSize = float2(light.Width, light.Height);

    return vout;
}

//------------------------  Pixel  ------------------------------//

GBufferPixelOut PSMain(VertexOut pin)
{
    bool pixelOutsideLightRadius = !IsPointInsideEllipse(pin.LocalSpacePosition, 0.xx, pin.LightSize);
    
    if (pixelOutsideLightRadius) 
    {
        discard;
    }

    return GetEmissiveGBufferPixelOutput(RootConstantBuffer.LightTableIndex, 0.0, pin.ViewDepth);
}

#endif