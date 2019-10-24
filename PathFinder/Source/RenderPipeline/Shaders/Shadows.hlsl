struct PassData
{

};

struct ShadowRayPayload
{
    float ShadowFactor;
};

#define PassDataType PassData

#include "BaseRootSignature.hlsl"
#include "InstanceData.hlsl"
#include "Vertices.hlsl"

RaytracingAccelerationStructure SceneBVH : register(t0, space0);
ConstantBuffer<InstanceData> InstanceTable : register(b0, space0);
StructuredBuffer<Vertex1P1N1UV1T1BT> Unified1P1N1UV1T1BTVertexBuffer : register(t1, space0);
StructuredBuffer<IndexU32> Unified1P1N1UV1T1BTIndexBuffer : register(t2, space0);

[shader("raygeneration")]
void RayGeneration()
{
    //float3 rayDir;
    //float3 origin;

    //// Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    //GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);

    //// Trace the ray.
    //// Set the ray's extents.
    //RayDesc ray;
    //ray.Origin = origin;
    //ray.Direction = rayDir;
    //// Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    //// TMin should be kept small to prevent missing geometry at close contact areas.
    //ray.TMin = 0.001;
    //ray.TMax = 10000.0;
    //RayPayload payload = { float4(0, 0, 0, 0) };
    //TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    //// Write the raytraced color to the output texture.
    //RenderTarget[DispatchRaysIndex().xy] = payload.color;
}

[shader("miss")]
void RayMiss(inout ShadowRayPayload payload)
{
   /* if (!payload.SkipShading)
    {
        g_screenOutput[DispatchRaysIndex().xy] = float4(0, 0, 0, 1);
    }*/
}