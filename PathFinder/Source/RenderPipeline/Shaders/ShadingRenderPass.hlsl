#ifndef _Shadows__
#define _Shadows__

#include "Exposure.hlsl"
#include "ColorConversion.hlsl"
#include "Mesh.hlsl"
#include "Vertices.hlsl"
#include "SpaceConversion.hlsl"
#include "Light.hlsl"
#include "GBuffer.hlsl"

static const uint MaxSupportedLights = 4;
static const uint RaysPerLight = 1;

struct ExposureOutput
{
    // Luminance value after exposition.
    float3 ExposedLuminance;

    // A luminance value that exceeded Saturation Based Sensitivity
    // of our virtual camera's sensor. 0 if no oversaturation occurred.
    float3 OversaturatedLuminance;
};

struct ShadowRayPayload
{
    float ShadowFactor;
};

struct ShadingResult
{
    float3 AnalyticalUnshadowedOutgoingLuminance;
    float3 StochasticUnshadowedOutgoingLuminance;
    float3 StochasticShadowedOutgoingLuminance;
};

struct PassData
{
    // 1 4-component set of Halton numbers for rays of each light
    float4 HaltonSequence[MaxSupportedLights];
    // 16 byte boundary
    uint BlueNoiseTextureIndex;
    uint AnalyticalLuminanceOutputTextureIndex;
    uint StochasticUnshadowedLuminanceOutputTextureIndex;
    uint StochasticShadowedLuminanceOutputTextureIndex;
    // 16 byte boundary
    uint2 BlueNoiseTextureSize;
    uint GBufferMaterialDataTextureIndex;
    uint GBufferDepthTextureIndex;
    // 16 byte boundary
    LightTablePartitionInfo LightOffsets;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

RaytracingAccelerationStructure SceneBVH : register(t0, space0);
StructuredBuffer<MeshInstance> InstanceTable : register(t1, space0);
StructuredBuffer<Vertex1P1N1UV1T1BT> UnifiedVertexBuffer : register(t2, space0);
StructuredBuffer<IndexU32> UnifiedIndexBuffer : register(t3, space0);
StructuredBuffer<Light> LightTable : register(t4, space0);
StructuredBuffer<Material> MaterialTable : register(t5, space0);

float4 RandomNumbersForLight(uint lightIndex, uint rayIndex, float4 blueNoise)
{
    // Halton sequence array must accommodate RaysPerLight number of sets
    float4 halton = PassDataCB.HaltonSequence[lightIndex * RaysPerLight + rayIndex];

    // Used for 2D position on light/BRDF
    float u1 = frac(halton.r + blueNoise.r);
    float u2 = frac(halton.g + blueNoise.g);

    // Choosing BRDF lobes
    float u3 = frac(halton.b + blueNoise.b);

    // Choosing between light and BRDF sampling
    float u4 = frac(halton.a + blueNoise.a);

    return float4(u1, u2, u3, u4);
}

ShadingResult ZeroShadingResult()
{
    ShadingResult result;
    result.AnalyticalUnshadowedOutgoingLuminance = 0.xxx;
    result.StochasticShadowedOutgoingLuminance = 0.xxx;
    result.StochasticUnshadowedOutgoingLuminance = 0.xxx;
    return result;
}

LTCTerms FetchLTCTerms(GBufferStandard gBuffer, Material material, float3 viewDirWS)
{
    Texture2D LTC_LUT_Specular_MatrixInverse = Textures2D[material.LTC_LUT_MatrixInverse_Specular_Index];
    Texture2D LTC_LUT_Specular_Matrix = Textures2D[material.LTC_LUT_Matrix_Specular_Index];
    Texture2D LTC_LUT_Specular_Terms = Textures2D[material.LTC_LUT_Terms_Specular_Index];

    Texture2D LTC_LUT_Diffuse_MatrixInverse = Textures2D[material.LTC_LUT_MatrixInverse_Diffuse_Index];
    Texture2D LTC_LUT_Diffuse_Matrix = Textures2D[material.LTC_LUT_Matrix_Diffuse_Index];
    Texture2D LTC_LUT_Diffuse_Terms = Textures2D[material.LTC_LUT_Terms_Diffuse_Index];

    float2 LTC_LUT_Size = float2(material.LTC_LUT_TextureSize, material.LTC_LUT_TextureSize);

    // Fetch precomputed matrices and fresnel/shadowing terms
    float NdotV = saturate(dot(gBuffer.Normal, viewDirWS));
    float2 uv = float2(gBuffer.Roughness, sqrt(1.0 - NdotV));
    uv = Refit0to1ValuesToTexelCenter(uv, LTC_LUT_Size);

    float4 inverseMatrixComponentsSpecular = LTC_LUT_Specular_MatrixInverse.SampleLevel(LinearClampSampler, uv, 0);
    float4 matrixComponentsSpecular = LTC_LUT_Specular_Matrix.SampleLevel(LinearClampSampler, uv, 0);
    float4 maskingFresnelAndScaleSpecular = LTC_LUT_Specular_Terms.SampleLevel(LinearClampSampler, uv, 0);

    float3x3 MInvSpecular = Matrix3x3ColumnMajor(
        float3(inverseMatrixComponentsSpecular.x, 0, inverseMatrixComponentsSpecular.y),
        float3(0, 1, 0),
        float3(inverseMatrixComponentsSpecular.z, 0, inverseMatrixComponentsSpecular.w)
    );

    float3x3 MSpecular = Matrix3x3ColumnMajor(
        float3(matrixComponentsSpecular.x, 0, matrixComponentsSpecular.y),
        float3(0, 1, 0),
        float3(matrixComponentsSpecular.z, 0, matrixComponentsSpecular.w)
    );

    float4 inverseMatrixComponentsDiffuse = LTC_LUT_Diffuse_MatrixInverse.SampleLevel(LinearClampSampler, uv, 0);
    float4 matrixComponentsDiffuse = LTC_LUT_Diffuse_Matrix.SampleLevel(LinearClampSampler, uv, 0);

    float3x3 MInvDiffuse = Matrix3x3ColumnMajor(
        float3(inverseMatrixComponentsDiffuse.x, 0, inverseMatrixComponentsDiffuse.y),
        float3(0, 1, 0),
        float3(inverseMatrixComponentsDiffuse.z, 0, inverseMatrixComponentsDiffuse.w)
    );

    float3x3 MDiffuse = Matrix3x3ColumnMajor(
        float3(matrixComponentsDiffuse.x, 0, matrixComponentsDiffuse.y),
        float3(0, 1, 0),
        float3(matrixComponentsDiffuse.z, 0, matrixComponentsDiffuse.w)
    );

    float geometryMasking = maskingFresnelAndScaleSpecular.x;
    float fresnel = maskingFresnelAndScaleSpecular.y;

    // Apply BRDF magnitude and Fresnel
    float3 f90 = lerp(0.04, gBuffer.Albedo, gBuffer.Metalness);
    float3 specularAlbedo = f90 * geometryMasking + (1.0 - f90) * fresnel;

    // Light that is not reflected is absorbed by conductors, so no diffuse term for them
    float3 diffuseAlbedo = gBuffer.Albedo * (1.0 - gBuffer.Metalness);

    // Construct orthonormal basis around N
    // World to tangent matrix
    float3x3 tangentToWorld = LookAtMatrix3x3(gBuffer.Normal, GetUpVectorForOrientaion(gBuffer.Normal));
    float3x3 worldToTangent = transpose(tangentToWorld);

    LTCTerms terms;
    terms.MInvSpecular = mul(worldToTangent, MInvSpecular);
    terms.MInvDiffuse = mul(worldToTangent, MInvDiffuse);
    terms.MSpecular = mul(MSpecular, tangentToWorld); // inv(AB) = inv(B)*inv(A)
    terms.MDiffuse = mul(MDiffuse, tangentToWorld); // inv(AB) = inv(B)*inv(A)
    terms.MDetSpecular = determinant(terms.MSpecular);
    terms.MDetDiffuse = determinant(terms.MDiffuse);
    terms.SurfaceSpecularAlbedo = specularAlbedo;
    terms.SurfaceDiffuseAlbedo = diffuseAlbedo;
    terms.LUT_Specular_Terms = LTC_LUT_Specular_Terms;
    terms.LUT_Diffuse_Terms = LTC_LUT_Diffuse_Terms;
    terms.LUTSize = material.LTC_LUT_TextureSize;
    terms.LUTSampler = LinearClampSampler;

    return terms;
}

float TraceShadowRay(Ray ray)
{
    // Trace the ray.
    // Set the ray's extents.
    RayDesc dxrRay;
    dxrRay.Origin = ray.Origin;
    dxrRay.Direction = ray.Direction;
    dxrRay.TMin = ray.TMin + 1e-03;
    dxrRay.TMax = ray.TMax - 1e-03;

    ShadowRayPayload shadowPayload = { 0.0f };

    TraceRay(SceneBVH,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
        | RAY_FLAG_FORCE_OPAQUE             // Skip any hit shaders
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // Skip closest hit shaders,
        0xFF, // Instance mask
        0, // Contribution to hit group index
        0, // BLAS geometry multiplier for hit group index
        0, // Miss shader index
        dxrRay, shadowPayload);

    return shadowPayload.ShadowFactor;
}

ShadingResult ShadeWithSphericalLights(GBufferStandard gBuffer, LTCTerms ltcTerms, float4 blueNoise, float3 viewDirection, float3 surfacePosition)
{
    uint lightsCount = PassDataCB.LightOffsets.SphericalLightsCount;
    ShadingResult shadingResult = ZeroShadingResult();

    for (uint lightIdx = 0; lightIdx < lightsCount; ++lightIdx)
    {
        uint lightTableOffset = lightIdx + PassDataCB.LightOffsets.SphericalLightsOffset;

        Light light = LightTable[lightIdx];
        LightPoints lightPoints = ComputeLightPoints(light, surfacePosition);
        ShericalLightSamplingInputs samplingInputs = ComputeSphericalLightSamplingInputs(light, surfacePosition);
        LTCEvaluationResult directLightingEvaluationResult = EvaluateDirectSphericalLighting(light, lightPoints, gBuffer, ltcTerms, viewDirection, surfacePosition);

        for (uint rayIdx = 0; rayIdx < RaysPerLight; ++rayIdx)
        {
            float4 randomNumbers = RandomNumbersForLight(lightTableOffset, rayIdx, blueNoise);
            
            // Pick a light sampling vector based on probability of taking a vector from BRDF distribution
            // versus picking a direct vector to one of random points on the light's surface
            float3 sampleVector = 0.0;

            // BRDF sample
            if (randomNumbers.w <= directLightingEvaluationResult.BRDFProbability)
            {
                // Randomly pick specular or diffuse lobe based on diffuse probability
                bool isSpecular = randomNumbers.z > directLightingEvaluationResult.DiffuseProbability;
                // Importance sample the BRDF
                float3x3 M = isSpecular ? ltcTerms.MSpecular : ltcTerms.MDiffuse;
                sampleVector = LTCSampleVector(M, randomNumbers.x, randomNumbers.y);

                //shadingResult.StochasticUnshadowedOutgoingLuminance = float3(0.0, 30.0, 0.0);
            }
            else // Light sample
            {
                sampleVector = SphereLightSampleVector(samplingInputs, randomNumbers.x, randomNumbers.y);
            }

            LightSample lightSample = ZeroLightSample();
            if (SampleSphereLight(light, samplingInputs, sampleVector, lightSample))
            {
                LTCSingleRayEvaluationResult brdfRayLightingEvaluationResult = EvaluateLTCLightingForSampleVector(light, ltcTerms, sampleVector, directLightingEvaluationResult.DiffuseProbability);
                float misPDF = lerp(lightSample.PDF, brdfRayLightingEvaluationResult.PDF, directLightingEvaluationResult.BRDFProbability);

                // This test should almost never fail because we just importance sampled from the BRDF.
                // Only an all black BRDF or a precision issue could trigger this.
                if (directLightingEvaluationResult.BRDFProbability > 0.0f)
                {
                    // Stochastic estimate of incident light, without shadow
                    // Note: projection cosine dot(w_i, n) is baked into the LTC-based BRDF
                    float3 luminance = brdfRayLightingEvaluationResult.OutgoingLuminance / misPDF;
                    shadingResult.StochasticUnshadowedOutgoingLuminance += luminance;

                    if (lightSample.LightToSurfaceRay.TMax > 1e-05)
                    {
                        float shadow = TraceShadowRay(lightSample.LightToSurfaceRay);
                        shadingResult.StochasticShadowedOutgoingLuminance += luminance * shadow;
                    }  
                }
            }
        }

        shadingResult.AnalyticalUnshadowedOutgoingLuminance += directLightingEvaluationResult.OutgoingLuminance;
    }

    return shadingResult;
}

float3 EvaluateStandardGBufferLighting(GBufferStandard gBuffer, float2 uv, uint2 pixelIndex, float depth)
{
    float3 output = 0.0;
    float totalSampleCount = PassDataCB.LightOffsets.TotalLightsCount * RaysPerLight;

    Texture2D blueNoiseTexture = Textures2D[PassDataCB.BlueNoiseTextureIndex];
    Material material = MaterialTable[gBuffer.MaterialIndex];

    float4 blueNoise = blueNoiseTexture.Load(uint3(pixelIndex % PassDataCB.BlueNoiseTextureSize, 0));
    float3 surfacePosition = ReconstructWorldPosition(depth, uv, FrameDataCB.CameraInverseView, FrameDataCB.CameraInverseProjection);
    float3 viewDirection = normalize(FrameDataCB.CameraPosition.xyz - surfacePosition);

    LTCTerms ltcTerms = FetchLTCTerms(gBuffer, material, viewDirection);

    ShadingResult sphericalLightsShadingResult = ShadeWithSphericalLights(gBuffer, ltcTerms, blueNoise, viewDirection, surfacePosition);

    output += 
        sphericalLightsShadingResult.AnalyticalUnshadowedOutgoingLuminance *
        sphericalLightsShadingResult.StochasticUnshadowedOutgoingLuminance /
        sphericalLightsShadingResult.StochasticShadowedOutgoingLuminance;
    output /= totalSampleCount;

    return output;
}

float3 EvaluateEmissiveGBufferLighting(GBufferEmissive gBuffer)
{
    return gBuffer.LuminousIntensity;
}

ExposureOutput ExposeOutgoingLuminance(float3 outgoingLuminance)
{
    float maxHsbsLuminance = ConvertEV100ToMaxHsbsLuminance(FrameDataCB.CameraExposureValue100);

    float3 HSV = RGBtoHSV(outgoingLuminance);
    HSV.z /= maxHsbsLuminance;

    float3 RGB = HSVtoRGB(HSV);

    ExposureOutput exposureOutput;
    exposureOutput.ExposedLuminance = RGB;
    exposureOutput.OversaturatedLuminance = HSV.z > 1.0 ? RGB : 0.0.xxx;

    return exposureOutput;
}

[shader("raygeneration")]
void RayGeneration()
{
    Texture2D<uint4> materialData = UInt4_Textures2D[PassDataCB.GBufferMaterialDataTextureIndex];
    Texture2D depthTexture = Textures2D[PassDataCB.GBufferDepthTextureIndex];
    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.AnalyticalLuminanceOutputTextureIndex];

    uint2 pixelIndex = DispatchRaysIndex().xy;
    float2 currenPixelLocation = pixelIndex + float2(0.5f, 0.5f);
    float2 pixelCenterUV = currenPixelLocation / DispatchRaysDimensions().xy;
    float depth = depthTexture.Load(uint3(pixelIndex, 0)).r;

    // Skip empty and emissive areas
    if (depth >= 1.0)
    {
        outputImage[pixelIndex].rgb = 0.xxx;
        return;
    } 

    GBufferEncoded encodedGBuffer;
    encodedGBuffer.MaterialData = materialData.Load(uint3(pixelIndex, 0));

    uint gBufferType = DecodeGBufferType(encodedGBuffer);
    float3 outgoingLuminance = 0.xxx;

    switch (gBufferType)
    {
    case GBufferTypeStandard:
        outgoingLuminance = EvaluateStandardGBufferLighting(DecodeGBufferStandard(encodedGBuffer), pixelCenterUV, pixelIndex, depth);
        break;

    case GBufferTypeEmissive:
        outgoingLuminance = EvaluateEmissiveGBufferLighting(DecodeGBufferEmissive(encodedGBuffer));
        break;
    }

    ExposureOutput output = ExposeOutgoingLuminance(outgoingLuminance);

    outputImage[pixelIndex] = float4(output.ExposedLuminance, 1.0);

    /*mainOutputImage[dispatchThreadID.xy] = float4(output.ExposedLuminance, 1.0);
    oversaturatedOutputImage[dispatchThreadID.xy] = float4(output.OversaturatedLuminance, 1.0);*/
}

[shader("miss")]
void RayMiss(inout ShadowRayPayload payload)
{
    payload.ShadowFactor = 1.0;
}

#endif