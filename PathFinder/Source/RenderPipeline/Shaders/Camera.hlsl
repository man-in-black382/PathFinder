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
    return camera.NearPlane * camera.FarPlane / (camera.FarPlane + hyperbolicDepth * (camera.NearPlane - camera.FarPlane));
}

float4 ReconstructViewSpacePosition(
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

    return viewSpacePosition;
}

float4 ReconstructWorldSpacePosition(
    float hyperbolicDepth, // Depth value after viewport transformation in [0; 1] range
    float2 ssuv, // Normalized screen-space (texture) coordinates [0; 1]
    Camera camera)
{
    float4 viewSpacePosition = ReconstructViewSpacePosition(hyperbolicDepth, ssuv, camera);
    float4 worldSpacePosition = mul(camera.InverseView, viewSpacePosition);

    return worldSpacePosition;
}

float3 WorldSpaceCameraRay(float2 centerUV, Camera camera)
{
    return normalize(ReconstructWorldSpacePosition(1.0, centerUV, camera).xyz - camera.Position.xyz);
}

#endif