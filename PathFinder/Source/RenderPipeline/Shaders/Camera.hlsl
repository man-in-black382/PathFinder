#ifndef _Camera__
#define _Camera__

struct Camera
{
    float4 Position;
    // 16 byte boundary
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float4x4 InverseView;
    float4x4 InverseProjection;
    float4x4 InverseViewProjection;
    // 16 byte boundary
    float NearPlane;
    float FarPlane;
    float ExposureValue100;
    uint __Pad0;
    // 16 byte boundary
};

float LinearizeDepth(float hyperbolicDepth, Camera camera)
{
    float n = camera.NearPlane;
    float f = camera.FarPlane;

    return n * f / (f + hyperbolicDepth * (n - f));
}

float3 ReconstructViewSpacePosition(
    float hyperbolicDepth, // Depth value after viewport transformation in [0; 1] range
    float2 ssuv, // Normalized screen-space (texture) coordinates [0; 1]
    Camera camera) // Inverse camera projection matrix
{
    // Have to invert Y due to DirectX convention for [0, 0] to be at the TOP left
    float2 uv = float2(ssuv.x, 1.0 - ssuv.y);

    float z = hyperbolicDepth;
    float2 xy = uv * 2.0 - 1.0;

    float4 clipSpacePosition = float4(xy, z, 1.0);
    float4 viewSpacePosition = mul(camera.InverseProjection, clipSpacePosition);

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}

float3 ReconstructWorldSpacePosition(
    float hyperbolicDepth, // Depth value after viewport transformation in [0; 1] range
    float2 ssuv, // Normalized screen-space (texture) coordinates [0; 1]
    Camera camera)
{
    float4 viewSpacePosition = float4(ReconstructViewSpacePosition(hyperbolicDepth, ssuv, camera), 1.0);
    float4 worldSpacePosition = mul(camera.InverseView, viewSpacePosition);

    return worldSpacePosition.xyz;
}

void ReconstructPositions(float hyperbolicDepth, float2 ssuv, Camera camera, out float3 viewPosition, out float3 worldPosition)
{
    float4 viewSpacePosition = float4(ReconstructViewSpacePosition(hyperbolicDepth, ssuv, camera), 1.0);
    float4 worldSpacePosition = mul(camera.InverseView, viewSpacePosition);

    viewPosition = viewSpacePosition.xyz;
    worldPosition = worldSpacePosition.xyz;
}

float3 WorldSpaceCameraRay(float2 centerUV, Camera camera)
{
    return normalize(ReconstructWorldSpacePosition(1.0, centerUV, camera) - camera.Position.xyz);
}

float3 ViewProjectPoint(float3 p, Camera camera)
{
    float4 projected = mul(camera.ViewProjection, float4(p, 1.0));
    projected /= projected.w;
    return projected.xyz;
}

float3 ProjectPoint(float3 p, Camera camera)
{
    float4 projected = mul(camera.Projection, float4(p, 1.0));
    projected /= projected.w;
    return projected.xyz;
}

#endif