float3 ReconstructWorldPosition(
    float hyperbolicDepth, // Depth value after viewport transformation in [0; 1] range
    float2 SSUV, // Normalized screen-space (texture) coordinates [0; 1]
    float4x4 inverseView, // Inverse camera view matrix
    float4x4 inverseProjection) // Inverse camera projection matrix
{
    float z = hyperbolicDepth;
    float2 xy = SSUV * 2.0 - 1.0;

    float4 clipSpacePosition = float4(xy, z, 1.0);
    float4 viewSpacePosition = mul(inverseProjection, clipSpacePosition);

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    float4 worldSpacePosition = mul(inverseView, viewSpacePosition);

    return worldSpacePosition.xyz;
}