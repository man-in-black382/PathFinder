#ifndef _SpaceConversion__
#define _SpaceConversion__

float3 ReconstructWorldPosition(
    float hyperbolicDepth, // Depth value after viewport transformation in [0; 1] range
    float2 SSUV, // Normalized screen-space (texture) coordinates [0; 1]
    float4x4 inverseView, // Inverse camera view matrix
    float4x4 inverseProjection) // Inverse camera projection matrix
{
    // Have to invert Y due to DirectX convention for [0, 0] to be at the TOP left
    float2 uv = float2(SSUV.x, 1.0 - SSUV.y); 

    float z = hyperbolicDepth;
    float2 xy = uv * 2.0 - 1.0;

    float4 clipSpacePosition = float4(xy, z, 1.0);
    float4 viewSpacePosition = mul(inverseProjection, clipSpacePosition);

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    float4 worldSpacePosition = mul(inverseView, viewSpacePosition);

    return worldSpacePosition.xyz;
}

float3 ComputeCameraRayWS(float2 centerUV, float3 cameraPosition, float4x4 inverseView, float4x4 inverseProjection)
{
    return normalize(ReconstructWorldPosition(1.0, centerUV, inverseView, inverseProjection).xyz - cameraPosition);
}

#endif