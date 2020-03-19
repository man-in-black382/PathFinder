#ifndef _Shadows__
#define _Shadows__

struct ShadowRayPayload
{
    float ShadowFactor;
};

struct PassData
{
    uint ShadowMaskTextureIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "Mesh.hlsl"
#include "Vertices.hlsl"
#include "SpaceConversion.hlsl"

RaytracingAccelerationStructure SceneBVH : register(t0, space0);
StructuredBuffer<MeshInstance> InstanceTable : register(t1, space0);
StructuredBuffer<Vertex1P1N1UV1T1BT> Unified1P1N1UV1T1BTVertexBuffer : register(t2, space0);
StructuredBuffer<IndexU32> Unified1P1N1UV1T1BTIndexBuffer : register(t3, space0);

[shader("raygeneration")]
void RayGeneration()
{
    uint2 pixelIndex = DispatchRaysIndex().xy;
    float2 currenPixelLocation = pixelIndex + float2(0.5f, 0.5f);
    float2 pixelCenterUV = currenPixelLocation / DispatchRaysDimensions().xy;

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = FrameDataCB.CameraPosition.xyz;
    ray.Direction = ComputeCameraRayWS(pixelCenterUV, FrameDataCB.CameraPosition, FrameDataCB.CameraInverseView, FrameDataCB.CameraInverseProjection);
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    //ray.Direction.y *= -1.0;
    ray.TMin = 0.001;
    ray.TMax = 100.0;

    ShadowRayPayload shadowPayload = { 0.0f };

    TraceRay(SceneBVH,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
        | RAY_FLAG_FORCE_OPAQUE             // Skip any hit shaders
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // Skip closest hit shaders,
        0xFF, // Instance mask
        0, // Contribution to hit group index
        0, // BLAS geometry multiplier for hit group index
        0, // Miss shader index
        ray, shadowPayload);

    RWTexture2D<float4> shadowMask = RW_Float4_Textures2D[PassDataCB.ShadowMaskTextureIndex];
    shadowMask[pixelIndex].rgb = float3(shadowPayload.ShadowFactor, 0.0, 0.0);
}

[shader("miss")]
void RayMiss(inout ShadowRayPayload payload)
{
    payload.ShadowFactor = 1.0;
}

#endif