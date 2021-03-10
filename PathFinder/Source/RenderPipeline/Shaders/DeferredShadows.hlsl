#ifndef _DeferredShadows__
#define _DeferredShadows__

#include "Mesh.hlsl"
#include "GBuffer.hlsl"
#include "CookTorrance.hlsl"

struct PassData
{
    GBufferTextureIndices GBufferIndices;
    // 16 byte boundary
    uint ShadowRayPDFsTexIdx;
    uint ShadowRayIntersectionPointsTexIdx;
    uint StochasticShadowedOutputTexIdx;
    uint StochasticUnshadowedOutputTexIdx;
};

struct Payload
{
    float Value;
};

#define PassDataType PassData

#include "ShadingCommon.hlsl"

void TraceShadowsStandardGBuffer(GBufferTexturePack gBufferTextures, float2 uv, uint2 pixelIndex, float depth)
{
    Texture2D rayPDFs = Textures2D[PassDataCB.ShadowRayPDFsTexIdx];
    Texture2D<uint4> rayLightIntersectionPoints = UInt4_Textures2D[PassDataCB.ShadowRayIntersectionPointsTexIdx];

    GBufferStandard gBuffer = ZeroGBufferStandard();
    LoadStandardGBuffer(gBuffer, gBufferTextures, pixelIndex);

    Material material = MaterialTable[gBuffer.MaterialIndex];

    float3 surfacePosition = NDCDepthToWorldPosition(depth, uv, FrameDataCB.CurrentFrameCamera);
    LightTablePartitionInfo partitionInfo = DecompressLightPartitionInfo();
    float3 viewDirection = normalize(FrameDataCB.CurrentFrameCamera.Position.xyz - surfacePosition);

    uint raysPerLight = RaysPerLight(partitionInfo);
    float4 pdfs = rayPDFs[pixelIndex];
    uint4 intersectionPoints = rayLightIntersectionPoints[pixelIndex];
    float3 shadowed = 0.0;
    float3 unshadowed = 0.0;

    const uint RayFlags = 
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
        RAY_FLAG_FORCE_OPAQUE |           // Skip any hit shaders
        RAY_FLAG_SKIP_CLOSEST_HIT_SHADER; // Skip closest hit shaders,

    [unroll]
    for (uint i = 0; i < TotalMaxRayCount; ++i)
    {
        if (intersectionPoints[i] == 0)
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

        float3 brdf = CookTorranceBRDF(
            gBuffer.Normal,
            viewDirection,
            normalize(viewDirection + gBuffer.Normal),
            normalize(-lightToSurface),
            gBuffer.Roughness,
            gBuffer.Albedo,
            gBuffer.Metalness,
            light.Luminance * light.Color.rgb
        );

        RayDesc dxrRay;
        dxrRay.Origin = lightIntersectionPoint;
        dxrRay.Direction = lightToSurface / vectorLength;
        dxrRay.TMin = tmin;
        dxrRay.TMax = tmax;

        Payload payload = { 0.0 };

        const int MissShaderIndex = 0;

        TraceRay(SceneBVH,
            RayFlags,
            EntityMaskMeshInstance, // Instance mask 
            0, // Contribution to hit group index
            0, // BLAS geometry multiplier for hit group index
            MissShaderIndex, // Miss shader index
            dxrRay, 
            payload);

        shadowed += brdf * payload.Value;
        unshadowed += brdf;
    }

    RW_Float4_Textures2D[PassDataCB.StochasticShadowedOutputTexIdx][pixelIndex].rgb = shadowed;
    RW_Float4_Textures2D[PassDataCB.StochasticUnshadowedOutputTexIdx][pixelIndex].rgb = unshadowed;
}

void OutputDefaults(uint2 pixelIndex)
{
    RW_Float4_Textures2D[PassDataCB.StochasticShadowedOutputTexIdx][pixelIndex].rgb = 1.0;
    RW_Float4_Textures2D[PassDataCB.StochasticUnshadowedOutputTexIdx][pixelIndex].rgb = 1.0;
}

[shader("closesthit")]
void RayClosestHit(inout Payload payload, BuiltInTriangleIntersectionAttributes attributes)
{
  
}

[shader("miss")]
void RayMiss(inout Payload payload)
{
    payload.Value = 1.0;
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
    
    TraceShadowsStandardGBuffer(gBufferTextures, pixelCenterUV, pixelIndex, depth);
}

#endif