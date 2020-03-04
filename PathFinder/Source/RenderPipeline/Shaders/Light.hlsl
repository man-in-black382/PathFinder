#ifndef _Light__
#define _Light__

#include "Matrix.hlsl"

static const uint LightTypeDisk = 0;
static const uint LightTypeSphere = 1;
static const uint LightTypeRectangle = 2;

struct Light
{
    float4 Orientation;
    float4 Position;
    float4 Color;
    float LuminousIntensity;
    float Luminance;
    float Width;
    float Height;
    uint LightType;

    uint Pad0__;
    uint Pad1__;
    uint Pad2__;
};

struct DiskLightPoints
{
    float3 Points[4];
};

DiskLightPoints GetLightPointsWS(Light light)
{
    DiskLightPoints points;

    float halfWidth = light.Width * 0.5; 
    float halfHeight = light.Height * 0.5; 

    float3 position = light.Position.xyz;
    float3 orientation = light.Orientation.xyz;

    // Get billboard points at the origin
    float4 p0 = float4(-halfWidth, -halfHeight, 0.0, 0.0);
    float4 p1 = float4(halfWidth, -halfHeight, 0.0, 0.0);
    float4 p2 = float4(halfWidth, halfHeight, 0.0, 0.0);
    float4 p3 = float4(-halfWidth, halfHeight, 0.0, 0.0);

    float4x4 diskRotation = LookAtMatrix(orientation, GetUpVectorForOrientaion(orientation));

    // Rotate around origin
    p0 = mul(diskRotation, p0);
    p1 = mul(diskRotation, p1);
    p2 = mul(diskRotation, p2);
    p3 = mul(diskRotation, p3);

    // Move points to light's location
    // Clockwise to match LTC convention
    points.Points[0] = p3.xyz + light.Position.xyz;
    points.Points[1] = p2.xyz + light.Position.xyz;
    points.Points[2] = p1.xyz + light.Position.xyz;
    points.Points[3] = p0.xyz + light.Position.xyz;

    return points;
}

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

    float4x4 diskRotation = LookAtMatrix(orientation, GetUpVectorForOrientaion(orientation));

    // Rotate around origin
    lightPoint = mul(diskRotation, lightPoint);

    // Move points to light's location
    worldSpaceCoord = lightPoint.xyz + light.Position.xyz;
}

#endif