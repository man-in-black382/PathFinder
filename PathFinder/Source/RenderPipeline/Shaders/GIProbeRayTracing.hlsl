#ifndef _Shading__
#define _Shading__

#include "Random.hlsl"
#include "Light.hlsl"
#include "Mesh.hlsl"
#include "GIProbeHelpers.hlsl"

struct ProbeRayPayload
{
    float Stub; 
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
    float3 T = ApplyBarycentrics(vertex0.Tangent, vertex1.Tangent, vertex2.Tangent, attributes.barycentrics);

    N = mul(instanceData.NormalMatrix, float4(normalize(N), 0.0)).xyz;
    T = mul(instanceData.NormalMatrix, float4(normalize(T), 0.0)).xyz;

    // If model is scaled, vectors will be scaled too, so renormalization is mandatory
    N = normalize(N);
    T = normalize(T);

    float3 B = normalize(cross(N, T));
    float3x3 TBN = Matrix3x3ColumnMajor(T, B, N);

    Texture2D albedoTexture = Textures2D[material.AlbedoMapIndex];
    Texture2D normalTexture = Textures2D[material.NormalMapIndex];
    Texture2D roughnessTexture = Textures2D[material.RoughnessMapIndex];
    Texture2D metalnessTexture = Textures2D[material.MetalnessMapIndex];

    GBufferStandard gBuffer;
    gBuffer.Albedo = SRGBToLinear(albedoTexture.SampleLevel(AnisotropicClampSampler(), uv, 0).rgb);
    gBuffer.Normal = mul(TBN, ExpandGBufferNormal(normalTexture.SampleLevel(AnisotropicClampSampler(), uv, 0).rgb));
    gBuffer.Roughness = roughnessTexture.SampleLevel(AnisotropicClampSampler(), uv, 0).r;
    gBuffer.Metalness = metalnessTexture.SampleLevel(AnisotropicClampSampler(), uv, 0).r;

    if (!instanceData.HasTangentSpace)
        gBuffer.Normal = N;

    uint rayIndex = DispatchRaysIndex().x;
    uint wrappedRayIndex = rayIndex % (PassDataCB.BlueNoiseTexSize.x * PassDataCB.BlueNoiseTexSize.y);

    RandomSequences randomSequences;
    randomSequences.BlueNoise = Textures2D[PassDataCB.BlueNoiseTexIdx][Index2DFrom1D(wrappedRayIndex, PassDataCB.BlueNoiseTexSize)];
    randomSequences.Halton = PassDataCB.Halton;

    uint probeIndex = Probe1DIndexFromRayIndex(rayIndex, PassDataCB.ProbeField);
    uint3 probe3DIndex = Probe3DIndexFrom1D(probeIndex, PassDataCB.ProbeField);
    float3 probePosition = ProbePositionFrom3DIndex(probe3DIndex, PassDataCB.ProbeField);
    float3 surfacePosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    //ShadingResult shadingResult = EvaluateStandardGBufferLighting(gBuffer, material, surfacePosition, probePosition, randomSequences);

    //RWTexture2D<float4> rayHitInfoOutputTexture = RW_Float4_Textures2D[PassDataCB.ProbeField.RayHitInfoTextureIdx];
    //uint2 outputTexelIdx = RayHitTexelIndex(rayIndex, probeIndex, PassDataCB.ProbeField);

    //rayHitInfoOutputTexture[outputTexelIdx] = float4(shadingResult.StochasticShadowedOutgoingLuminance, RayTCurrent());
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
    OutputResult(float4(0.0, 0.0, 0.0, GITRayMiss));
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

    const int MissShaderIndex = 0; 

    TraceRay(SceneBVH,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        | RAY_FLAG_FORCE_OPAQUE, // Skip any hit shaders
        EntityMaskMeshInstance, // Instance mask 
        0, // Contribution to hit group index
        0, // BLAS geometry multiplier for hit group index
        MissShaderIndex, // Miss shader index
        dxrRay, payload);

    //OutputResult(float4(rayDir.x > 0 ? rayDir.x * 10 : 0, 0, 0, 10.0));
}

#endif