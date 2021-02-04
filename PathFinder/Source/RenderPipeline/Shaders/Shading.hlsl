#ifndef _Shading__
#define _Shading__

#include "Exposure.hlsl"
#include "Mesh.hlsl"
#include "GBuffer.hlsl"

struct PassData
{
    GBufferTextureIndices GBufferIndices;
    float4 Halton;
    // 16 byte boundary
    uint BlueNoiseTexIdx;
    uint AnalyticOutputTexIdx;
    uint StochasticShadowedOutputTexIdx;
    uint StochasticUnshadowedOutputTexIdx;
    // 16 byte boundary
    uint2 BlueNoiseTextureSize;
    uint RngSeedsTexIdx;
    uint FrameNumber;
};

#define PassDataType PassData

#include "ShadingCommon.hlsl"

ShadingResult HandleStandardGBufferLighting(GBufferTexturePack gBufferTextures, float2 uv, uint2 pixelIndex, float depth)
{
    Texture3D blueNoiseTexture = Textures3D[PassDataCB.BlueNoiseTexIdx];
    Texture2D<uint4> rngSeeds = UInt4_Textures2D[PassDataCB.RngSeedsTexIdx];

    GBufferStandard gBuffer = ZeroGBufferStandard();
    LoadStandardGBuffer(gBuffer, gBufferTextures, pixelIndex);

    Material material = MaterialTable[gBuffer.MaterialIndex];

    RandomSequences randomSequences;
    randomSequences.BlueNoise = blueNoiseTexture[rngSeeds[pixelIndex].xyz];
    randomSequences.Halton = PassDataCB.Halton;

    float3 surfacePosition = NDCDepthToWorldPosition(depth, uv, FrameDataCB.CurrentFrameCamera);

    return EvaluateStandardGBufferLighting(gBuffer, material, surfacePosition, FrameDataCB.CurrentFrameCamera.Position.xyz, randomSequences);
}

ShadingResult HandleEmissiveGBufferLighting(GBufferTexturePack gBufferTextures, uint2 pixelIndex)
{
    GBufferEmissive gBuffer = ZeroGBufferEmissive();
    LoadEmissiveGBuffer(gBuffer, gBufferTextures, pixelIndex);

    Light light = LightTable[gBuffer.LightIndex];

    ShadingResult result = ZeroShadingResult();
    result.AnalyticUnshadowedOutgoingLuminance = light.Luminance * light.Color.rgb;
    return result;
}

void OutputShadingResult(ShadingResult shadingResult, uint2 pixelIndex)
{
    RWTexture2D<float4> analyticOutput = RW_Float4_Textures2D[PassDataCB.AnalyticOutputTexIdx];
    RWTexture2D<float4> stochasticUnshadowedOutput = RW_Float4_Textures2D[PassDataCB.StochasticUnshadowedOutputTexIdx];
    RWTexture2D<float4> stochasticShadowedOutput = RW_Float4_Textures2D[PassDataCB.StochasticShadowedOutputTexIdx];

    analyticOutput[pixelIndex] = float4(shadingResult.AnalyticUnshadowedOutgoingLuminance, 1.0);
    stochasticShadowedOutput[pixelIndex] = float4(shadingResult.StochasticShadowedOutgoingLuminance, 1.0);
    stochasticUnshadowedOutput[pixelIndex] = float4(shadingResult.StochasticUnshadowedOutgoingLuminance, 1.0);
}

//[shader("raygeneration")]
//void RayGeneration()

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID)
{
    GBufferTexturePack gBufferTextures;
    gBufferTextures.AlbedoMetalness = Textures2D[PassDataCB.GBufferIndices.AlbedoMetalnessTexIdx];
    gBufferTextures.NormalRoughness = Textures2D[PassDataCB.GBufferIndices.NormalRoughnessTexIdx];
    gBufferTextures.Motion = UInt4_Textures2D[PassDataCB.GBufferIndices.MotionTexIdx];
    gBufferTextures.TypeAndMaterialIndex = UInt4_Textures2D[PassDataCB.GBufferIndices.TypeAndMaterialTexIdx];
    gBufferTextures.DepthStencil = Textures2D[PassDataCB.GBufferIndices.DepthStencilTexIdx];
    gBufferTextures.ViewDepth = Textures2D[PassDataCB.GBufferIndices.ViewDepthTexIdx];

    //uint2 pixelIndex = DispatchRaysIndex().xy;
    //float2 currenPixelLocation = pixelIndex + float2(0.5f, 0.5f);
    //float2 pixelCenterUV = currenPixelLocation / DispatchRaysDimensions().xy;

    uint2 pixelIndex = dispatchThreadID.xy;
    float2 pixelCenterUV = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);

    float depth = gBufferTextures.DepthStencil.Load(uint3(pixelIndex, 0)).r;

    ShadingResult shadingResult = ZeroShadingResult();

    // Skip empty and emissive areas
    if (depth >= 1.0)
    {
        OutputShadingResult(shadingResult, pixelIndex);
        return;
    }

    uint gBufferType = LoadGBufferType(gBufferTextures, pixelIndex);

    switch (gBufferType)
    {
    case GBufferTypeStandard:
        shadingResult = HandleStandardGBufferLighting(gBufferTextures, pixelCenterUV, pixelIndex, depth);
        break;

    case GBufferTypeEmissive:
        shadingResult = HandleEmissiveGBufferLighting(gBufferTextures, pixelIndex);
        break;
    }

    OutputShadingResult(shadingResult, pixelIndex);
}

#endif