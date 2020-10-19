#ifndef _GeometryPicking__
#define _GeometryPicking__

struct PassData
{
    uint2 MousePosition;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "Constants.hlsl"

struct IntersectionInfo
{
    // ID that will help us get intersected geometry on CPU
    uint InstanceID;
};

RaytracingAccelerationStructure SceneBVH : register(t0, space0);
RWStructuredBuffer<IntersectionInfo> IntersectionInfoBuffer : register(u0, space0);

[shader("closesthit")]
void RayClosestHit(inout IntersectionInfo payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    payload.InstanceID = InstanceID();
}

[shader("raygeneration")]
void RayGeneration()
{
    uint2 pixelIdx = PassDataCB.MousePosition;
    float2 uv = (pixelIdx + float2(0.5f, 0.5f)) / GlobalDataCB.PipelineRTResolution;

    float3 direction = WorldCameraRay(uv, FrameDataCB.CurrentFrameCamera);

    RayDesc dxrRay;
    dxrRay.Origin = FrameDataCB.CurrentFrameCamera.Position.xyz;
    dxrRay.Direction = direction;
    dxrRay.TMin = FrameDataCB.CurrentFrameCamera.NearPlane;
    dxrRay.TMax = FrameDataCB.CurrentFrameCamera.FarPlane;

    IntersectionInfo payload = { U32Max };

    TraceRay(SceneBVH,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
        | RAY_FLAG_FORCE_OPAQUE, // Skip any hit shaders
        EntityMaskAll, // Instance mask
        0, // Contribution to hit group index
        0, // BLAS geometry multiplier for hit group index
        0, // Miss shader index
        dxrRay, payload);

    IntersectionInfoBuffer[0] = payload;
}

#endif