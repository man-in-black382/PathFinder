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

float3 EvaluateDirectLighting(Light light, float3 V, float3 surfaceWPos, GBufferStandard gBuffer, Texture2D LTC_LUT0, Texture2D LTC_LUT1, float2 LTC_LUT_Size)
{
    // Integrate area light
    float3 specular = 0.0.xxx;
    float3 diffuse = 0.0.xxx;
    
    // Fetch precomputed matrices and fresnel/shadowing terms
    float NdotV = saturate(dot(gBuffer.Normal, V));
    float2 uv = float2(gBuffer.Roughness, sqrt(1.0 - NdotV));
    uv = Refit0to1ValuesToTexelCenter(uv, LTC_LUT_Size);

    float4 t1 = LTC_LUT0.SampleLevel(LinearClampSampler, uv, 0);
    float4 t2 = LTC_LUT1.SampleLevel(LinearClampSampler, uv, 0);

    float3x3 Minv = Matrix3x3ColumnMajor(
        float3(t1.x, 0, t1.y),
        float3(0, 1, 0),
        float3(t1.z, 0, t1.w)
    );

    float geometryMasking = t2.x;
    float fresnel = t2.y;
    
    // Integrate area lights
    switch (light.LightType)
    {  
        case LightTypeSphere:
        {
            // Spherical light is just a disk light always oriented towards surface
            light.Orientation = float4(0.0, -1.0, 0.0, 0.0);// float4(normalize(surfaceWPos - light.Position.xyz), 0.0);
            // Proceed to disk integration
        }
        
        case LightTypeDisk:
        {
            DiskLightPoints diskPoints = GetLightPointsWS(light);
            specular = LTCEvaluateDisk(gBuffer.Normal, V, surfaceWPos, Minv, diskPoints.Points, LTC_LUT1, LTC_LUT_Size, LinearClampSampler);
            diffuse = LTCEvaluateDisk(gBuffer.Normal, V, surfaceWPos, Matrix3x3Identity, diskPoints.Points, LTC_LUT1, LTC_LUT_Size, LinearClampSampler);
            break;
        }
            
        case LightTypeRectangle:
        {
            DiskLightPoints diskPoints = GetLightPointsWS(light);
            specular = LTCEvaluateRectangle(gBuffer.Normal, V, surfaceWPos, Minv, diskPoints.Points);
            diffuse = LTCEvaluateRectangle(gBuffer.Normal, V, surfaceWPos, Matrix3x3Identity, diskPoints.Points);
            break;
        }

        default:
            return float3(1.0, 0.0, 0.0);
    }
    
    // BRDF shadowing and Fresnel
    float3 f90 = lerp(0.04, gBuffer.Albedo, gBuffer.Metalness);
    specular *= f90 * geometryMasking + (1.0 - f90) * fresnel;
    
    //  
    diffuse *= gBuffer.Albedo * (1.0 - gBuffer.Metalness) * (1.0 - fresnel);

    return (specular + diffuse) * light.Color.rgb * light.LuminousIntensity;
}

float3 EvaluateStandardGBufferLighting(GBufferStandard gBuffer, float2 uv, float depth)
{
    Material material = MaterialTable[gBuffer.MaterialIndex];

    Texture2D LTC_LUT0 = Textures2D[material.LTC_LUT_0_Specular_Index];
    Texture2D LTC_LUT1 = Textures2D[material.LTC_LUT_1_Specular_Index];
    float2 LTC_LUT_Size = float2(material.LTC_LUT_TextureSize, material.LTC_LUT_TextureSize);

    float3 worldPosition = ReconstructWorldPosition(depth, uv, FrameDataCB.CameraInverseView, FrameDataCB.CameraInverseProjection);
    float3 V = normalize(FrameDataCB.CameraPosition.xyz - worldPosition);

    float3 outgoingRadiance = 0.xxx;

    for (uint lightIdx = 0; lightIdx < RootConstantBuffer.TotalLightCount; ++lightIdx)
    {
        Light light = LightTable[lightIdx];
        outgoingRadiance += EvaluateDirectLighting(light, V, worldPosition, gBuffer, LTC_LUT0, LTC_LUT1, LTC_LUT_Size);
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

    outputImage[dispatchThreadID.xy] = float4(outgoingLuminance, 1.0);
}

#endif