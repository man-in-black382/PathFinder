#ifndef _DeferredLightingRenderPass__
#define _DeferredLightingRenderPass__

struct PassData
{
    uint GBufferMaterialDataTextureIndex;
    uint GBufferDepthTextureIndex;
    uint OutputTextureIndex;
};

#define PassDataType PassData

struct RootConstants
{
    uint TotalLightCount;
};

#include "MandatoryEntryPointInclude.hlsl"
#include "GBuffer.hlsl"
#include "ColorConversion.hlsl"
#include "CookTorrance.hlsl"
#include "SpaceConversion.hlsl"
#include "Utils.hlsl"
#include "Matrix.hlsl"
#include "Mesh.hlsl"
#include "Light.hlsl"
#include "LTC.hlsl"

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);
StructuredBuffer<Light> LightTable : register(t0);
StructuredBuffer<Material> MaterialTable : register(t1);

float3 EvaluateDirectLighting(Light light, float3 V, float3 surfaceWPos, GBufferStandard gBuffer, Material material)
{
    // Integrate area light
    float3 specular = 0.0.xxx;
    float3 diffuse = 0.0.xxx;

    Texture2D LTC_LUT_Specular_0 = Textures2D[material.LTC_LUT_0_Specular_Index];
    Texture2D LTC_LUT_Specular_1 = Textures2D[material.LTC_LUT_1_Specular_Index];

    Texture2D LTC_LUT_Diffuse_0 = Textures2D[material.LTC_LUT_0_Diffuse_Index];
    Texture2D LTC_LUT_Diffuse_1 = Textures2D[material.LTC_LUT_1_Diffuse_Index];

    float2 LTC_LUT_Size = float2(material.LTC_LUT_TextureSize, material.LTC_LUT_TextureSize);

    // Fetch precomputed matrices and fresnel/shadowing terms
    float NdotV = saturate(dot(gBuffer.Normal, V));
    float2 uv = float2(gBuffer.Roughness, sqrt(1.0 - NdotV));
    uv = Refit0to1ValuesToTexelCenter(uv, LTC_LUT_Size);

    float4 matrixComponentsSpecular = LTC_LUT_Specular_0.SampleLevel(LinearClampSampler, uv, 0);
    float4 maskingFresnelAndScaleSpecular = LTC_LUT_Specular_1.SampleLevel(LinearClampSampler, uv, 0);

    float3x3 MinvSpecular = Matrix3x3ColumnMajor(
        float3(matrixComponentsSpecular.x, 0, matrixComponentsSpecular.y),
        float3(0, 1, 0),
        float3(matrixComponentsSpecular.z, 0, matrixComponentsSpecular.w)
    );

    float4 matrixComponentsDiffuse = LTC_LUT_Diffuse_0.SampleLevel(LinearClampSampler, uv, 0);

    float3x3 MinvDiffuse = Matrix3x3ColumnMajor(
        float3(matrixComponentsDiffuse.x, 0, matrixComponentsDiffuse.y),
        float3(0, 1, 0),
        float3(matrixComponentsDiffuse.z, 0, matrixComponentsDiffuse.w)
    );

    float geometryMasking = maskingFresnelAndScaleSpecular.x;
    float fresnel = maskingFresnelAndScaleSpecular.y;

    // Integrate area lights
    switch (light.LightType)
    {  
        case LightTypeSphere:
        {
            // Spherical light is just a disk light always oriented towards surface
            light.Orientation = float4(normalize(surfaceWPos - light.Position.xyz), 0.0);
            // Proceed to disk integration
        }
        
        case LightTypeDisk:
        {
            // Add a small value so that width and height is never truly equal.
            // LTC disk evaluation code has nasty numerical errors (NaNs) in a corner case 
            // when width and height are fully equal and disk is rotated
            // strictly toward the surface point.
            // A slightly more wide light is almost imperceptible, so I guess it will have to do.
            //
            light.Width += 0.1;

            DiskLightPoints diskPoints = GetLightPointsWS(light);

            specular = LTCEvaluateDisk(gBuffer.Normal, V, surfaceWPos, MinvSpecular, diskPoints.Points, LTC_LUT_Specular_1, LTC_LUT_Size, LinearClampSampler);
            diffuse = LTCEvaluateDisk(gBuffer.Normal, V, surfaceWPos, MinvDiffuse, diskPoints.Points, LTC_LUT_Diffuse_1, LTC_LUT_Size, LinearClampSampler);
            break;
        }
            
        case LightTypeRectangle:
        {
            DiskLightPoints diskPoints = GetLightPointsWS(light);
            specular = LTCEvaluateRectangle(gBuffer.Normal, V, surfaceWPos, MinvSpecular, diskPoints.Points);
            diffuse = LTCEvaluateRectangle(gBuffer.Normal, V, surfaceWPos, MinvDiffuse, diskPoints.Points);
            break;
        }

        default:
            break;
    }
    
    // BRDF shadowing and Fresnel
    float3 f90 = lerp(0.04, gBuffer.Albedo, gBuffer.Metalness);
    specular *= f90 * geometryMasking + (1.0 - f90) * fresnel;
    
    // Light that is not reflected is absorbed by conductors, so no diffuse term for them
    diffuse *= gBuffer.Albedo * (1.0 - gBuffer.Metalness);

    return (specular + diffuse) * light.Color.rgb * light.LuminousIntensity;
}

float3 EvaluateStandardGBufferLighting(GBufferStandard gBuffer, float2 uv, float depth)
{
    Material material = MaterialTable[gBuffer.MaterialIndex];
    float3 worldPosition = ReconstructWorldPosition(depth, uv, FrameDataCB.CameraInverseView, FrameDataCB.CameraInverseProjection);
    float3 V = normalize(FrameDataCB.CameraPosition.xyz - worldPosition);

    float3 outgoingRadiance = 0.xxx;

    for (uint lightIdx = 0; lightIdx < RootConstantBuffer.TotalLightCount; ++lightIdx)
    {
        Light light = LightTable[lightIdx];
        outgoingRadiance += EvaluateDirectLighting(light, V, worldPosition, gBuffer, material);
    }

    return outgoingRadiance;
}

float3 EvaluateEmissiveGBufferLighting(GBufferEmissive gBuffer)
{
    return gBuffer.LuminousIntensity;
}

[numthreads(32, 32, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{   
    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];

    Texture2D<uint4> materialData = UInt4_Textures2D[PassDataCB.GBufferMaterialDataTextureIndex];
    Texture2D depthTexture = Textures2D[PassDataCB.GBufferDepthTextureIndex];

    uint2 pixelIndex = dispatchThreadID.xy;
    float2 UV = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);
    float depth = depthTexture.Load(uint3(pixelIndex, 0)).r;

    // Skip empty areas
    if (depth >= 1.0)
    {
        outputImage[dispatchThreadID.xy] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    GBufferEncoded encodedGBuffer;
    encodedGBuffer.MaterialData = materialData.Load(uint3(pixelIndex, 0));

    uint gBufferType = DecodeGBufferType(encodedGBuffer);
    float3 outgoingLuminance = 0.xxx;

    switch (gBufferType)
    {
    case GBufferTypeStandard: 
        outgoingLuminance = EvaluateStandardGBufferLighting(DecodeGBufferStandard(encodedGBuffer), UV, depth); 
        break;

    case GBufferTypeEmissive:
        outgoingLuminance = EvaluateEmissiveGBufferLighting(DecodeGBufferEmissive(encodedGBuffer));
        break;
    }

    outputImage[dispatchThreadID.xy] = float4(1.0, 0.0, 0.0/*outgoingLuminance*/, 1.0);
}

#endif