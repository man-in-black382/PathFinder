#ifndef _Shading__
#define _Shading__

#include "Random.hlsl"
#include "Light.hlsl"
#include "Mesh.hlsl"
#include "GIProbeHelpers.hlsl"

struct ProbeRayPayload
{
    float Value; 
};

struct PassData
{
    IrradianceField ProbeField;
    float4 Halton;
    uint2 BlueNoiseTexSize;
    uint BlueNoiseTexIdx;
};

#define PassDataType PassData
#define SHADING_STOCHASTIC_ONLY

#include "ShadingCommon.hlsl"

void OutputResult(float4 value)
{
    uint rayIndex = DispatchRaysIndex().x;
    uint probeIndex = Probe1DIndexFromRayIndex(rayIndex, PassDataCB.ProbeField);

    RWTexture2D<float4> rayHitInfoOutputTexture = RW_Float4_Textures2D[PassDataCB.ProbeField.RayHitInfoTextureIdx];
    uint2 outputTexelIdx = RayHitTexelIndex(rayIndex, probeIndex, PassDataCB.ProbeField);

    rayHitInfoOutputTexture[outputTexelIdx] = value;
}

[shader("closesthit")]
void MeshRayClosestHit(inout ProbeRayPayload payload, BuiltInTriangleIntersectionAttributes attributes)
{
    uint instanceIdx = InstanceID();
    uint triangleIdx = PrimitiveIndex();
    uint vertexIndex0 = triangleIdx * 3;

    MeshInstance instanceData = MeshInstanceTable[instanceIdx];
    Material material = MaterialTable[instanceData.MaterialIndex];

    // Load index and vertex
    uint index0 = UnifiedIndexBuffer[instanceData.UnifiedIndexBufferOffset + vertexIndex0];
    uint index1 = UnifiedIndexBuffer[instanceData.UnifiedIndexBufferOffset + vertexIndex0 + 1];
    uint index2 = UnifiedIndexBuffer[instanceData.UnifiedIndexBufferOffset + vertexIndex0 + 2];

    Vertex1P1N1UV1T1BT vertex0 = UnifiedVertexBuffer[instanceData.UnifiedVertexBufferOffset + index0];
    Vertex1P1N1UV1T1BT vertex1 = UnifiedVertexBuffer[instanceData.UnifiedVertexBufferOffset + index1];
    Vertex1P1N1UV1T1BT vertex2 = UnifiedVertexBuffer[instanceData.UnifiedVertexBufferOffset + index2];

    float3 debugPosition = ApplyBarycentrics(vertex0.Position.xyz, vertex1.Position.xyz, vertex2.Position.xyz, attributes.barycentrics);
    debugPosition = mul(instanceData.ModelMatrix, float4(debugPosition, 1.0)).xyz;

    float2 uv = ApplyBarycentrics(vertex0.UV, vertex1.UV, vertex2.UV, attributes.barycentrics);
    float3 N = ApplyBarycentrics(vertex0.Normal, vertex1.Normal, vertex2.Normal, attributes.barycentrics);

    Texture2D albedoTexture = Textures2D[material.AlbedoMapIndex];
    Texture2D normalTexture = Textures2D[material.NormalMapIndex];
    Texture2D roughnessTexture = Textures2D[material.RoughnessMapIndex];
    Texture2D metalnessTexture = Textures2D[material.MetalnessMapIndex];

    GBufferStandard gBuffer;

    gBuffer.Albedo = all(material.DiffuseAlbedoOverride < 0.0) ?
        SRGBToLinear(albedoTexture.SampleLevel(AnisotropicClampSampler(), uv, 0).rgb) :
        material.DiffuseAlbedoOverride;

    gBuffer.Normal = N;

    gBuffer.Roughness = material.RoughnessOverride < 0.0 ?
        roughnessTexture.SampleLevel(AnisotropicClampSampler(), uv, 0).r :
        material.RoughnessOverride;

    gBuffer.Metalness = material.RoughnessOverride < 0.0 ?
        metalnessTexture.SampleLevel(AnisotropicClampSampler(), uv, 0).r :
        material.RoughnessOverride;

    uint rayIndex = DispatchRaysIndex().x;
    uint wrappedRayIndex = rayIndex % (PassDataCB.BlueNoiseTexSize.x * PassDataCB.BlueNoiseTexSize.y);

    RandomSequences randomSequences;
    randomSequences.BlueNoise = Textures2D[PassDataCB.BlueNoiseTexIdx][Index2DFrom1D(wrappedRayIndex, PassDataCB.BlueNoiseTexSize)];
    randomSequences.Halton = PassDataCB.Halton;

    uint probeIndex = Probe1DIndexFromRayIndex(rayIndex, PassDataCB.ProbeField);
    uint3 probe3DIndex = Probe3DIndexFrom1D(probeIndex, PassDataCB.ProbeField);
    float3 probePosition = ProbePositionFrom3DIndex(probe3DIndex, PassDataCB.ProbeField);
    float3 surfacePosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3x3 surfaceTangentToWorld = RotationMatrix3x3(gBuffer.Normal);
    float3x3 surfaceWorldToTangent = transpose(surfaceTangentToWorld);

    LightTablePartitionInfo partitionInfo = DecompressLightPartitionInfo();
    float3 viewDirection = normalize(FrameDataCB.CurrentFrameCamera.Position.xyz - surfacePosition);
    float3 wo = mul(surfaceWorldToTangent, viewDirection);
    ShadingResult shadingResult = ZeroShadingResult();

    ShadeWithSphericalLights(gBuffer, partitionInfo, randomSequences, wo, surfacePosition, surfaceTangentToWorld, surfaceWorldToTangent, shadingResult);
    ShadeWithRectangularLights(gBuffer, partitionInfo, randomSequences, wo, surfacePosition, surfaceTangentToWorld, surfaceWorldToTangent, shadingResult);
    ShadeWithEllipticalLights(gBuffer, partitionInfo, randomSequences, wo, surfacePosition, surfaceTangentToWorld, surfaceWorldToTangent, shadingResult);

    Texture2D irradianceAtlas = Textures2D[PassDataCB.ProbeField.PreviousIrradianceProbeAtlasTexIdx];
    Texture2D depthAtlas = Textures2D[PassDataCB.ProbeField.PreviousDepthProbeAtlasTexIdx];
    float3 irradiance = RetrieveGIIrradiance(surfacePosition, gBuffer.Normal, viewDirection, irradianceAtlas, depthAtlas, LinearClampSampler(), PassDataCB.ProbeField, true);

    float3 shadowed = DiffuseBRDFForGI(viewDirection, gBuffer) * irradiance;

    [unroll]
    for (uint i = 0; i < TotalMaxRayCount; ++i)
    {
        uint lightIndex = i / RaysPerLight(partitionInfo);
        Light light = LightTable[lightIndex];
        float3 lightIntersectionPoint = 0.0;
        float3x3 lightRotation = RotationMatrix3x3(light.Orientation.xyz);

        switch (light.LightType)
        {
        case LightTypeSphere: lightIntersectionPoint = UnpackRaySphericalLightIntersectionPoint(shadingResult.RayLightIntersectionData[i], light); break;
        case LightTypeRectangle: lightIntersectionPoint = UnpackRayRectangularLightIntersectionPoint(shadingResult.RayLightIntersectionData[i], light, lightRotation); break;
        case LightTypeEllipse: lightIntersectionPoint = UnpackRayDiskLightIntersectionPoint(shadingResult.RayLightIntersectionData[i], light, lightRotation); break;
        }
        
        // Skipped missed or otherwise problematic rays
        if (all(shadingResult.StochasticUnshadowedOutgoingLuminance[i]) <= 0.0)
            continue;

        float3 lightToSurface = surfacePosition - lightIntersectionPoint;
        float vectorLength = length(lightToSurface);
        float tmax = vectorLength - 1e-03;
        float tmin = 1e-03;

        RayDesc dxrRay;
        dxrRay.Origin = lightIntersectionPoint;
        dxrRay.Direction = lightToSurface / vectorLength;
        dxrRay.TMin = tmin;
        dxrRay.TMax = tmax;

        ProbeRayPayload payload = { 0.0 };

        // Select SecondaryShadowRayMiss(...)
        const int MissShaderIndex = 1;
        const uint RayFlags =
            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
            RAY_FLAG_FORCE_OPAQUE |           // Skip any hit shaders
            RAY_FLAG_SKIP_CLOSEST_HIT_SHADER; // Skip closest hit shaders,

        TraceRay(SceneBVH,
            RayFlags,
            EntityMaskMeshInstance, // Instance mask 
            0, // Contribution to hit group index
            0, // BLAS geometry multiplier for hit group index
            MissShaderIndex, // Miss shader index
            dxrRay,
            payload);

        shadowed += shadingResult.StochasticUnshadowedOutgoingLuminance[i] * payload.Value;
    }

    RWTexture2D<float4> rayHitInfoOutputTexture = RW_Float4_Textures2D[PassDataCB.ProbeField.RayHitInfoTextureIdx];
    uint2 outputTexelIdx = RayHitTexelIndex(rayIndex, probeIndex, PassDataCB.ProbeField);

    bool hitBackFace = dot(gBuffer.Normal, WorldRayDirection()) >= 0.0;
    rayHitInfoOutputTexture[outputTexelIdx] = float4(shadowed, hitBackFace ? ProbeRayBackfaceIndicator : RayTCurrent());
}

[shader("closesthit")]
void LightRayClosestHit(inout ProbeRayPayload payload, BuiltInTriangleIntersectionAttributes attributes)
{
    uint instanceIdx = InstanceID();
    Light light = LightTable[instanceIdx];
    OutputResult(float4(light.Luminance * light.Color.rgb, RayTCurrent()));
}

[shader("miss")]
void ProbeRayMiss(inout ProbeRayPayload payload)
{
    float maxRayLength = length(PassDataCB.ProbeField.CellSize.xxx);
    OutputResult(float4(0.0, 0.0, 0.0, FloatMax));
}

[shader("miss")]
void SecondaryShadowRayMiss(inout ProbeRayPayload payload)
{
    payload.Value = 1.0;
}

[shader("raygeneration")]
void RayGeneration()
{
    uint rayIndex = DispatchRaysIndex().x;
    float3 probePosition = ProbePositionFromRayIndex(rayIndex, PassDataCB.ProbeField);
    float3 rayDir = ProbeSamplingVector(rayIndex, PassDataCB.ProbeField);

    RayDesc dxrRay;
    dxrRay.Origin = probePosition;
    dxrRay.Direction = rayDir;
    dxrRay.TMin = 1e-03;
    dxrRay.TMax = FloatMax;

    ProbeRayPayload payload = { 0 };

    // Select ProbeRayMiss(...)
    const int MissShaderIndex = 0; 

    TraceRay(SceneBVH,
        RAY_FLAG_FORCE_OPAQUE, // Skip any hit shaders
        EntityMaskMeshInstance, // Instance mask 
        0, // Contribution to hit group index
        0, // BLAS geometry multiplier for hit group index
        MissShaderIndex, // Miss shader index
        dxrRay, payload);
}

#endif