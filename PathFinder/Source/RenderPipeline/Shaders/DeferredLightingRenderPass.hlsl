#ifndef _DeferredLightingRenderPass__
#define _DeferredLightingRenderPass__

struct PassData
{
    uint GBufferMaterialDataTextureIndex;
    uint GBufferDepthTextureIndex;
    uint OutputTextureIndex;
    uint LTC_LUT0_Index;
    uint LTC_LUT1_Index;
    uint LTC_LUT_Size;
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
#include "InstanceData.hlsl"
#include "LTC.hlsl"

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);
StructuredBuffer<LightInstanceData> LightInstanceTable : register(t0);

DiskLightPoints GetFlatLightPoints(LightInstanceData light)
{
    DiskLightPoints points;

    float halfWidth = light.Width * 0.5;
    float halfHeight = light.Height * 0.5;

    float3 position = light.Position.xyz;
    float3 orientation = light.Orientation.xyz;

    // Get billboard points at the origin
    float4 p0 = float4(-halfWidth, -halfHeight, 0.0, 0.0);
    float4 p1 = float4(halfWidth, -halfHeight, 0.0, 0.0);
    float4 p2 = float4(halfWidth, halfHeight, 0.0, 0.0);
    float4 p3 = float4(-halfWidth, halfHeight, 0.0, 0.0);

    float4x4 diskRotation = LookAtMatrix(orientation, GetUpVectorForOrientaion(orientation));

    // Rotate around origin
    p0 = mul(diskRotation, p0);
    p1 = mul(diskRotation, p1);
    p2 = mul(diskRotation, p2);
    p3 = mul(diskRotation, p3);

    // Move points to light's location
    points.Points[0] = p0.xyz + light.Position.xyz;
    points.Points[1] = p1.xyz + light.Position.xyz;
    points.Points[2] = p2.xyz + light.Position.xyz;
    points.Points[3] = p3.xyz + light.Position.xyz;

    return points;
}

float3 EvaluateDirectLighting(LightInstanceData light, float3 V, float3 surfaceWPos, GBuffer gBuffer, Texture2D LTC_LUT0, Texture2D LTC_LUT1)
{
    // Integrate area light
    float2 LUTSize = float2(PassDataCB.LTC_LUT_Size, PassDataCB.LTC_LUT_Size);
    float3 specular = 0.0.xxx;
    float3 diffuse = 0.0.xxx;
    
    // Fetch precomputed matrices and fresnel/shadowing terms
    float NdotV = saturate(dot(gBuffer.Normal, V));
    float2 uv = float2(gBuffer.Roughness, sqrt(1.0 - NdotV));
    uv = Refit0to1ValuesToTexelCenter(uv, LUTSize);

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
            DiskLightPoints diskPoints = GetFlatLightPoints(light); 
            specular = LTCEvaluateDisk(gBuffer.Normal, V, surfaceWPos, Minv, diskPoints.Points, LTC_LUT1, LUTSize, LinearClampSampler);
            diffuse = LTCEvaluateDisk(gBuffer.Normal, V, surfaceWPos, Matrix3x3Identity, diskPoints.Points, LTC_LUT1, LUTSize, LinearClampSampler);
            break;
        }
            
        case LightTypeRectangle:
        {
            DiskLightPoints diskPoints = GetFlatLightPoints(light);
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

[numthreads(32, 32, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{   
    Texture2D LTC_LUT0 = Textures2D[PassDataCB.LTC_LUT0_Index];
    Texture2D LTC_LUT1 = Textures2D[PassDataCB.LTC_LUT1_Index];
    Texture2D<uint4> materialData = UInt4_Textures2D[PassDataCB.GBufferMaterialDataTextureIndex];
    Texture2D depthTexture = Textures2D[PassDataCB.GBufferDepthTextureIndex];

    uint2 pixelIndex = dispatchThreadID.xy;
    float2 UV = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);

    GBufferEncoded encodedGBuffer;
    encodedGBuffer.MaterialData = materialData.Load(uint3(pixelIndex, 0));
    
    GBuffer gBuffer = DecodeGBuffer(encodedGBuffer);

    float depth = depthTexture.Load(uint3(dispatchThreadID.xy, 0)).r;
    float3 worldPosition = ReconstructWorldPosition(depth, UV, FrameDataCB.CameraInverseView, FrameDataCB.CameraInverseProjection);
    float3 V = normalize(FrameDataCB.CameraPosition.xyz - worldPosition);

    float3 outgoingRadiance = 0.xxx;
    
    for (uint lightIdx = 0; lightIdx < RootConstantBuffer.TotalLightCount; ++lightIdx)
    {
        LightInstanceData light = LightInstanceTable[lightIdx];
        outgoingRadiance += EvaluateDirectLighting(light, V, worldPosition, gBuffer, LTC_LUT0, LTC_LUT1);
    }

    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];
    outputImage[dispatchThreadID.xy] = float4(outgoingRadiance, 1.0);
}

#endif