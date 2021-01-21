#ifndef _GIDebug__
#define _GIDebug__

#include "MandatoryEntryPointInclude.hlsl"
#include "Matrix.hlsl"

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
    //Light light = LightTable[RootConstantBuffer.LightTableIndex];

    //// Load index and vertex
    //uint index = UnifiedIndexBuffer[light.UnifiedIndexBufferOffset + vertexId];
    //Vertex1P1N1UV1T1BT vertex = UnifiedVertexBuffer[light.UnifiedVertexBufferOffset + index];

    //float2 localSpacePosition = vertex.Position.xy;

    //float4 WSPosition = mul(light.ModelMatrix, vertex.Position);
    //float4 CSPosition = mul(FrameDataCB.CurrentFrameCamera.View, WSPosition);
    //float4 ClipSPosition = mul(FrameDataCB.CurrentFrameCamera.Projection, CSPosition);

    VertexOut vout;
    //vout.Position = ClipSPosition;
    //vout.LightOrientation = light.Orientation.xyz;
    //vout.ViewDepth = CSPosition.z;
    //vout.LocalSpacePosition = light.LightType != LightTypeRectangle ? localSpacePosition : 0.xx;
    //vout.LightSize = float2(light.Width, light.Height);

    return vout;
}

//------------------------  Pixel  ------------------------------//

//GBufferPixelOut PSMain(VertexOut pin)
//{
//    bool pixelOutsideLightRadius = !IsPointInsideEllipse(pin.LocalSpacePosition, 0.xx, pin.LightSize);
//    
//    if (pixelOutsideLightRadius) 
//    {
//        discard;
//    }
//
//    return GetEmissiveGBufferPixelOutput(RootConstantBuffer.LightTableIndex, 0.0, pin.ViewDepth);
//}

#endif