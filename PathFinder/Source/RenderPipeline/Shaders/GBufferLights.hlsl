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

//------------------------  Vertex  ------------------------------//

struct VertexOut
{
    float4 Position : SV_POSITION;
    float3 LuminousIntensity : LUMINANCE;
    float2 LocalSpacePosition : DIST_FROM_CENTER;
    float2 LightSize : LIGHT_SIZE;
};

void GetLightVertexWS(Light light, uint vertexId, out float3 worldSpaceCoord, out float2 localSpaceCoord)
{
    float halfWidth = light.Width * 0.5;
    float halfHeight = light.Height * 0.5;

    float3 position = light.Position.xyz;
    float3 orientation = light.Orientation.xyz;

    // Get billboard points at the origin
    float dx = (vertexId == 0 || vertexId == 3) ? -halfWidth : halfWidth;
    float dy = (vertexId == 0 || vertexId == 1) ? -halfHeight : halfHeight;

    localSpaceCoord = float2(dx, dy);
    float4 lightPoint = float4(dx, dy, 0.0f, 0.0f);

    float4x4 diskRotation = RotationMatrix4x4(orientation, GetUpVectorForOrientaion(orientation));

    // Rotate around origin
    lightPoint = mul(diskRotation, lightPoint);

    // Move points to light's location
    worldSpaceCoord = lightPoint.xyz + light.Position.xyz;
}

VertexOut VSMain(uint vertexId : SV_VertexID)
{
    uint lightVertexIndex = vertexId;
    lightVertexIndex = lightVertexIndex == 3 ? 2 : lightVertexIndex;
    lightVertexIndex = lightVertexIndex == 4 ? 3 : lightVertexIndex;
    lightVertexIndex = lightVertexIndex == 5 ? 0 : lightVertexIndex;

    Light light = LightTable[RootConstantBuffer.LightTableIndex];

    // Sphere light is a disk always oriented towards the camera
    if (light.LightType == LightTypeSphere)
    {
        light.Orientation = float4(normalize(FrameDataCB.CameraPosition.xyz - light.Position.xyz), 0);
    }

    float2 localSpacePosition;
    float3 worldSpacePosition;
    GetLightVertexWS(light, lightVertexIndex, worldSpacePosition, localSpacePosition);

    float4 lightCornerVertex = float4(worldSpacePosition, 1.0);

    VertexOut vout;
    vout.Position = mul(FrameDataCB.CameraViewProjection, lightCornerVertex);
    vout.LuminousIntensity = light.LuminousIntensity * light.Color.rgb;
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

    GBufferEmissive gBuffer;
    gBuffer.LuminousIntensity = pin.LuminousIntensity;

    GBufferEncoded encodedGBuffer = EncodeGBuffer(gBuffer);

    GBufferPixelOut pixelOut;
    pixelOut.MaterialData = encodedGBuffer.MaterialData;
    return pixelOut;
}

#endif