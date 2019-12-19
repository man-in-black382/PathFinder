struct ShadowRayPayload
{
    float ShadowFactor;
};

#include "MandatoryEntryPointInclude.hlsl"
#include "InstanceData.hlsl"
#include "Vertices.hlsl"

RaytracingAccelerationStructure SceneBVH : register(t0, space0);
StructuredBuffer<InstanceData> InstanceTable : register(t1, space0);
StructuredBuffer<Vertex1P1N1UV1T1BT> Unified1P1N1UV1T1BTVertexBuffer : register(t2, space0);
StructuredBuffer<IndexU32> Unified1P1N1UV1T1BTIndexBuffer : register(t3, space0);

[shader("raygeneration")]
void RayGeneration()
{
    float3 rayDir = normalize(float3(1.0, 1.0, 1.0));
    float3 origin = float3(0.0, 0.0, 0.0);

    //// Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    //GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = 100.0;

    ShadowRayPayload shadowPayload = { 1.0f };

    TraceRay(SceneBVH,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
        | RAY_FLAG_FORCE_OPAQUE             // Skip any hit shaders
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // Skip closest hit shaders,
        0, // Instance mask
        0, // Contribution to hit group index
        0, // BLAS geometry multiplier for hit group index
        0, // Miss shader index
        ray, shadowPayload);
}

[shader("miss")]
void RayMiss(inout ShadowRayPayload payload)
{
    payload.ShadowFactor = 0.0;
}