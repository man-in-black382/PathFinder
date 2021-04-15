#ifndef _DeferredShadows__
#define _DeferredShadows__

#include "Mesh.hlsl"
#include "GBuffer.hlsl"
#include "BRDF.hlsl"

struct PassData
{
    GBufferTextureIndices GBufferIndices;
    // 16 byte boundary
    uint ShadowRayPDFsTexIdx;
    uint ShadowRayIntersectionPointsTexIdx;
    uint StochasticShadowedOutputTexIdx;
    uint StochasticUnshadowedOutputTexIdx;
    // 16 byte boundary
    uint BlueNoiseTexSize;
    uint BlueNoiseTexDepth;
    uint BlueNoiseTexIdx;
    uint FrameNumber;
};

struct Payload
{
    float Value;
};

#define PassDataType PassData

#include "ShadingCommon.hlsl"

void TraceForStandardGBuffer(GBufferTexturePack gBufferTextures, float2 uv, uint2 pixelIndex, float depth)
{
    Texture2D rayPDFs = Textures2D[PassDataCB.ShadowRayPDFsTexIdx];
    Texture2D<uint4> rayLightIntersectionPoints = UInt4_Textures2D[PassDataCB.ShadowRayIntersectionPointsTexIdx];

    GBufferStandard gBuffer = ZeroGBufferStandard();
    LoadStandardGBuffer(gBuffer, gBufferTextures, pixelIndex);

    Material material = MaterialTable[gBuffer.MaterialIndex];

    float3 surfacePosition = NDCDepthToWorldPosition(depth, uv, FrameDataCB.CurrentFrameCamera);
    LightTablePartitionInfo partitionInfo = DecompressLightPartitionInfo();
    float3 viewDirection = normalize(FrameDataCB.CurrentFrameCamera.Position.xyz - surfacePosition);
    float3x3 worldToTangent = transpose(RotationMatrix3x3(gBuffer.Normal));

    uint raysPerLight = RaysPerLight(partitionInfo);
    float4 pdfs = rayPDFs[pixelIndex];
    uint4 intersectionPoints = rayLightIntersectionPoints[pixelIndex];
    float4 shadowed = 0.0; // 4th component includes AO
    float3 unshadowed = 0.0;

    const uint ShadowRayFlags = 
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
        RAY_FLAG_FORCE_OPAQUE |           // Skip any hit shaders
        RAY_FLAG_SKIP_CLOSEST_HIT_SHADER; // Skip closest hit shaders,

    // Shadows

    [unroll]
    for (uint i = 0; i < TotalMaxRayCount; ++i)
    {
        if (intersectionPoints[i] == 0 || pdfs[i] < 0.0001)
            continue;

        uint lightIndex = i / raysPerLight;
        Light light = LightTable[lightIndex];
        float3 lightIntersectionPoint = 0.0;
        float3x3 lightRotation = RotationMatrix3x3(light.Orientation.xyz);

        switch (light.LightType)
        {
        case LightTypeSphere: lightIntersectionPoint = UnpackRaySphericalLightIntersectionPoint(intersectionPoints[i], light); break;
        case LightTypeRectangle: lightIntersectionPoint = UnpackRayRectangularLightIntersectionPoint(intersectionPoints[i], light, lightRotation); break;
        case LightTypeEllipse: lightIntersectionPoint = UnpackRayDiskLightIntersectionPoint(intersectionPoints[i], light, lightRotation); break;
        }

        float3 lightToSurface = surfacePosition - lightIntersectionPoint;
        float vectorLength = length(lightToSurface);
        float tmax = vectorLength - 1e-03;
        float tmin = 1e-03;

        float3 wo = mul(worldToTangent, viewDirection);
        float3 wi = mul(worldToTangent, normalize(-lightToSurface));
        float3 wm = normalize(wo + wi);

        float3 brdf = CookTorranceBRDF(wo, wi, wm, gBuffer)* light.Luminance* light.Color.rgb / pdfs[i] / raysPerLight;

        RayDesc shadowRay;
        shadowRay.Origin = lightIntersectionPoint;
        shadowRay.Direction = lightToSurface / vectorLength;
        shadowRay.TMin = tmin;
        shadowRay.TMax = tmax;

        Payload payload = { 0.0 };

        const int MissShaderIndex = 0;

        TraceRay(SceneBVH,
            ShadowRayFlags,
            EntityMaskMeshInstance, // Instance mask 
            0, // Contribution to hit group index
            0, // BLAS geometry multiplier for hit group index
            MissShaderIndex, // Miss shader index
            shadowRay, 
            payload);

        shadowed.rgb += brdf * payload.Value;
        unshadowed += brdf;
    }

    // Ambient Occlusion
    Texture3D blueNoiseTexture = Textures3D[PassDataCB.BlueNoiseTexIdx];
    uint3 blueNoiseTexelIndex = uint3(pixelIndex % PassDataCB.BlueNoiseTexSize, PassDataCB.FrameNumber % PassDataCB.BlueNoiseTexDepth);
    float4 blueNoise = blueNoiseTexture[blueNoiseTexelIndex];
    float3x3 tangentToWorld = RotationMatrix3x3(gBuffer.Normal);

    float AOMaxRayLength = 2.0;

    const uint AORayFlags = 
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
        RAY_FLAG_FORCE_OPAQUE |           // Skip any hit shaders
        RAY_FLAG_SKIP_CLOSEST_HIT_SHADER; // Skip closest hit shaders,

    RayDesc aoRay;
    aoRay.Origin = surfacePosition;
    aoRay.Direction = mul(tangentToWorld, UniformHemisphereSample(blueNoise.x, blueNoise.y));
    aoRay.TMin = 0.001;
    aoRay.TMax = AOMaxRayLength;

    Payload payload = { 0.0 };

    const int MissShaderIndex = 1; // AORayMiss

    TraceRay(SceneBVH,
        AORayFlags,
        EntityMaskMeshInstance, // Instance mask 
        0, // Contribution to hit group index
        0, // BLAS geometry multiplier for hit group index
        MissShaderIndex, // Miss shader index
        aoRay,
        payload);

    shadowed.w = payload.Value;

    RW_Float4_Textures2D[PassDataCB.StochasticShadowedOutputTexIdx][pixelIndex] = shadowed;
    RW_Float4_Textures2D[PassDataCB.StochasticUnshadowedOutputTexIdx][pixelIndex].rgb = unshadowed;
}

[shader("miss")]
void ShadowRayMiss(inout Payload payload)
{
    payload.Value = 1.0;
}

[shader("miss")]
void AORayMiss(inout Payload payload)
{
    payload.Value = 1.0;
}

void OutputDefaults(uint2 pixelIndex)
{
    RW_Float4_Textures2D[PassDataCB.StochasticShadowedOutputTexIdx][pixelIndex].rgb = 1.0;
    RW_Float4_Textures2D[PassDataCB.StochasticUnshadowedOutputTexIdx][pixelIndex].rgb = 1.0;
}

[shader("raygeneration")]
void RayGeneration()
{
    GBufferTexturePack gBufferTextures;
    gBufferTextures.AlbedoMetalness = Textures2D[PassDataCB.GBufferIndices.AlbedoMetalnessTexIdx];
    gBufferTextures.NormalRoughness = Textures2D[PassDataCB.GBufferIndices.NormalRoughnessTexIdx];
    gBufferTextures.Motion = UInt4_Textures2D[PassDataCB.GBufferIndices.MotionTexIdx];
    gBufferTextures.TypeAndMaterialIndex = UInt4_Textures2D[PassDataCB.GBufferIndices.TypeAndMaterialTexIdx];
    gBufferTextures.DepthStencil = Textures2D[PassDataCB.GBufferIndices.DepthStencilTexIdx];
    gBufferTextures.ViewDepth = Textures2D[PassDataCB.GBufferIndices.ViewDepthTexIdx];

    uint2 pixelIndex = DispatchRaysIndex().xy;
    float2 pixelCenterUV = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);
    float depth = gBufferTextures.DepthStencil.Load(uint3(pixelIndex, 0)).r;

    // Skip empty and emissive areas
    if (depth >= 1.0)
    {
        OutputDefaults(pixelIndex);
        return;
    }

    uint gBufferType = LoadGBufferType(gBufferTextures, pixelIndex);

    if (gBufferType == GBufferTypeEmissive)
    {
        OutputDefaults(pixelIndex);
        return;
    }
    
    TraceForStandardGBuffer(gBufferTextures, pixelCenterUV, pixelIndex, depth);
}

#endif