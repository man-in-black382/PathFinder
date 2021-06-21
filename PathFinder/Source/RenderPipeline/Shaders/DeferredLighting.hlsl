#ifndef _DeferredLighting__
#define _DeferredLighting__

#include "Mesh.hlsl"
#include "GBuffer.hlsl"
#include "Packing.hlsl"
#include "Exposure.hlsl"

struct PassData
{
    GBufferTextureIndices GBufferIndices;
    float4 Halton;
    // 16 byte boundary
    uint BlueNoiseTexIdx;
    uint AnalyticOutputTexIdx;
    uint ShadowRayPDFsTexIdx;
    uint ShadowRayIntersectionPointsTexIdx;
    // 16 byte boundary
    uint2 BlueNoiseTextureSize;
    uint RngSeedsTexIdx;
    uint FrameNumber;
    // 16 byte boundary
    uint SkyTexIdx;
};

#define PassDataType PassData

#include "ShadingCommon.hlsl"

ShadingResult HandleStandardGBufferLighting(GBufferTexturePack gBufferTextures, float2 uv, uint2 pixelIndex, float viewDepth)
{
    Texture3D blueNoiseTexture = Textures3D[PassDataCB.BlueNoiseTexIdx];
    Texture2D<uint4> rngSeeds = UInt4_Textures2D[PassDataCB.RngSeedsTexIdx];

    GBufferStandard gBuffer = ZeroGBufferStandard();
    LoadStandardGBufferNoMotion(gBuffer, gBufferTextures, pixelIndex);

    Material material = MaterialTable[gBuffer.MaterialIndex];

    RandomSequences randomSequences;
    randomSequences.BlueNoise = blueNoiseTexture[rngSeeds[pixelIndex].xyz];
    randomSequences.Halton = PassDataCB.Halton;

    float3 surfacePosition = ViewDepthToWorldPosition(viewDepth, uv, FrameDataCB.CurrentFrameCamera);
    LightTablePartitionInfo partitionInfo = DecompressLightPartitionInfo();
    float3 viewDirection = normalize(FrameDataCB.CurrentFrameCamera.Position.xyz - surfacePosition);
    float3x3 surfaceWorldToTangent = transpose(RotationMatrix3x3(gBuffer.Normal));
    LTCTerms ltcTerms = FetchLTCTerms(gBuffer, material, viewDirection);
    ShadingResult shadingResult = ZeroShadingResult();

    ShadeWithSun(gBuffer, partitionInfo, randomSequences, viewDirection, surfacePosition, surfaceWorldToTangent, shadingResult);
    ShadeWithSphericalLights(gBuffer, ltcTerms, partitionInfo, randomSequences, viewDirection, surfacePosition, shadingResult);
    ShadeWithRectangularLights(gBuffer, ltcTerms, partitionInfo, randomSequences, viewDirection, surfacePosition, shadingResult);
    ShadeWithEllipticalLights(gBuffer, ltcTerms, partitionInfo, randomSequences, viewDirection, surfacePosition, shadingResult);

    //shadingResult.AnalyticUnshadowedOutgoingLuminance = gBuffer.Normal * 100;

    return shadingResult;
}

ShadingResult HandleEmissiveGBufferLighting(GBufferTexturePack gBufferTextures, uint2 pixelIndex)
{
    GBufferEmissive gBuffer = ZeroGBufferEmissive();
    LoadEmissiveGBufferNoMotion(gBuffer, gBufferTextures, pixelIndex);

    Light light = LightTable[gBuffer.LightIndex];

    ShadingResult shadingResult = ZeroShadingResult();
    shadingResult.AnalyticUnshadowedOutgoingLuminance = light.Luminance * light.Color.rgb;

    return shadingResult;
}

ShadingResult GetSkyShadingResult(float2 pixelUV)
{
    Texture2D skyLuminanceTexture = Textures2D[PassDataCB.SkyTexIdx];
    float3 pointInfronOfCamera = NDCDepthToWorldPosition(1.0, pixelUV, FrameDataCB.CurrentFrameCamera);
    float3 worldViewDirection = normalize(pointInfronOfCamera - FrameDataCB.CurrentFrameCamera.Position.xyz);
    float2 skyUV = (OctEncode(worldViewDirection) + 1.0) * 0.5;
    float3 luminance = skyLuminanceTexture.SampleLevel(LinearClampSampler(), skyUV, 0.0).rgb;

    ShadingResult result = ZeroShadingResult();
    result.AnalyticUnshadowedOutgoingLuminance = luminance;
    return result;
}

void OutputShadingResult(ShadingResult shadingResult, uint2 pixelIndex)
{
    RW_Float4_Textures2D[PassDataCB.AnalyticOutputTexIdx][pixelIndex].rgb = shadingResult.AnalyticUnshadowedOutgoingLuminance;
    RW_Float4_Textures2D[PassDataCB.ShadowRayPDFsTexIdx][pixelIndex] = shadingResult.RayPDFs;
    RW_UInt4_Textures2D[PassDataCB.ShadowRayIntersectionPointsTexIdx][pixelIndex] = shadingResult.RayLightIntersectionData;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID)
{
    // ========================================================================================================================== //
    GBufferTexturePack gBufferTextures;
    gBufferTextures.AlbedoMetalness = Textures2D[PassDataCB.GBufferIndices.AlbedoMetalnessTexIdx];
    gBufferTextures.NormalRoughness = Textures2D[PassDataCB.GBufferIndices.NormalRoughnessTexIdx];
    gBufferTextures.TypeAndMaterialIndex = UInt4_Textures2D[PassDataCB.GBufferIndices.TypeAndMaterialTexIdx];
    gBufferTextures.ViewDepth = Textures2D[PassDataCB.GBufferIndices.ViewDepthTexIdx];
    // ========================================================================================================================== //

    uint2 pixelIndex = dispatchThreadID.xy;
    float2 pixelCenterUV = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);
    float viewDepth = gBufferTextures.ViewDepth.mips[0][pixelIndex].r;

    // Skip empty areas
    if (viewDepth >= FrameDataCB.CurrentFrameCamera.FarPlane)
    {
        OutputShadingResult(GetSkyShadingResult(pixelCenterUV), pixelIndex);
        return;
    }

    uint gBufferType = LoadGBufferType(gBufferTextures, pixelIndex);

    switch (gBufferType)
    {
    case GBufferTypeStandard: OutputShadingResult(HandleStandardGBufferLighting(gBufferTextures, pixelCenterUV, pixelIndex, viewDepth), pixelIndex); break;
    case GBufferTypeEmissive: OutputShadingResult(HandleEmissiveGBufferLighting(gBufferTextures, pixelIndex), pixelIndex); break;
    }
}

#endif