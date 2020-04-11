#ifndef _Shadows__
#define _Shadows__

#include "Exposure.hlsl"
#include "ColorConversion.hlsl"
#include "Mesh.hlsl"
#include "Vertices.hlsl"
#include "SpaceConversion.hlsl"
#include "Light.hlsl"
#include "GBuffer.hlsl"

// Turing-level hardware can realistically work with 4 lights and 1 ray per light, per tile
// We must forget about more to make space for other rendering workloads.
static const uint MaxSupportedLights = 4;
static const uint RaysPerLight = 1;
static const float RaysPerLightInverse = 1.0 / RaysPerLight;

struct ExposureOutput
{
    // Luminance value after exposition.
    float3 ExposedLuminance;

    // A luminance value that exceeded Saturation Based Sensitivity
    // of our virtual camera's sensor. 0 if no over saturation occurred.
    float3 OversaturatedLuminance;
};

struct ShadowRayPayload
{
    float ShadowFactor;
};

struct ShadingResult
{
    float3 AnalyticUnshadowedOutgoingLuminance;
    float3 StochasticUnshadowedOutgoingLuminance;
    float3 StochasticShadowedOutgoingLuminance;
};

// Store an absolute minimum of data required for ray tracing.
// We must be very careful with how many registers we occupy.
struct RTCache
{
    uint4 LightStatichticsData;
    float3 LightIntersectionPoints[MaxSupportedLights * RaysPerLight];
};

struct PassData
{
    // 1 4-component set of Halton numbers for rays of each light
    float4 HaltonSequence[MaxSupportedLights * RaysPerLight];
    // 16 byte boundary
    uint BlueNoiseTextureIndex;
    uint AnalyticOutputTextureIndex;
    uint StochasticUnshadowedOutputTextureIndex;
    uint StochasticShadowedOutputTextureIndex;
    // 16 byte boundary
    uint2 BlueNoiseTextureSize;
    uint GBufferMaterialDataTextureIndex;
    uint GBufferDepthTextureIndex;
};

struct RootConstants
{
    uint CompressedLightPartitionInfo;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);
RaytracingAccelerationStructure SceneBVH : register(t0, space0);
StructuredBuffer<Light> LightTable : register(t1, space0);
StructuredBuffer<Material> MaterialTable : register(t2, space0);

// An implementation of Combining Analytic Direct Illumination and Stochastic Shadows
// http://casual-effects.com/research/Heitz2018Shadow/Heitz2018SIGGRAPHTalk.pdf
// https://hal.archives-ouvertes.fr/hal-01761558/file/heitzI3D2018_slides.pdf
// https://research.nvidia.com/sites/default/files/pubs/2018-05_Combining-Analytic-Direct//I3D2018_combining.pdf

LightTablePartitionInfo DecompressLightPartitionInfo()
{
    // Light table offsets are supplied through root constants to speed up access
    // Lights are expected to be placed in order: spherical -> rectangular -> disk
    // Offsets are encoded in 8 bits each which means there can be no more than 256 lights total
    uint compressed = RootConstantBuffer.CompressedLightPartitionInfo;
    uint sphericalLightsOffset = (compressed >> 24) & 0xFF;
    uint rectangularLightsOffset = (compressed >> 16) & 0xFF;
    uint diskLightsOffset = (compressed >> 8) & 0xFF;    
    uint totalLightsCount = compressed & 0xFF;

    uint sphericalLightsCount = rectangularLightsOffset - sphericalLightsOffset;
    uint rectangularLightsCount = diskLightsOffset - rectangularLightsOffset;
    uint diskLightsCount = totalLightsCount - rectangularLightsCount - sphericalLightsCount;

    LightTablePartitionInfo info;
    info.DiskLightsCount = diskLightsCount;
    info.DiskLightsOffset = diskLightsOffset;
    info.RectangularLightsCount = rectangularLightsCount;
    info.RectangularLightsOffset = rectangularLightsOffset;
    info.SphericalLightsCount = sphericalLightsCount;
    info.SphericalLightsOffset = sphericalLightsOffset;
    info.TotalLightsCount = totalLightsCount;

    return info;
}

RTCache ZeroRTCache()
{
    RTCache cache;

    [unroll]
    for (uint i = 0; i < RaysPerLight * MaxSupportedLights; ++i)
    {
        //cache.LightIntersectionPoints[i] = 0.xxx;
        //cache.StochasticOutgoingLuminances[i] = 0.xxx;
    }
    return cache;
}

void SetRTCacheProbabilities(inout RTCache cache, uint lightIndex, float brdfProbability, float diffuseProbability, float lightSamplePDF)
{
    // Compress BRDF probability to 8 bits, diffuse prob. and light PDF to 12 bits each to pack them into a single UINT
    // As PDFs and probabilities are linear normalized values they are good candidates for fixed-point compression
    uint brdfProbabilityU8 = brdfProbability * U8Max;
    uint diffuseProbabilityU12 = diffuseProbability * U12Max;
    uint lightPDFU12 = lightSamplePDF * U12Max;

    uint compressed = 0;
    compressed |= brdfProbabilityU8 << 24;
    compressed |= diffuseProbabilityU12 << 12;
    compressed |= lightPDFU12;

    cache.LightStatichticsData[lightIndex] = compressed;
}

void SetRTCacheLightIntersectinPoint(inout RTCache cache, uint lightIndex, float3 intersectionPoint)
{
    cache.LightIntersectionPoints[lightIndex] = intersectionPoint;
}

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
    result.AnalyticUnshadowedOutgoingLuminance = 0.xxx;
    result.StochasticShadowedOutgoingLuminance = 0.xxx;
    result.StochasticUnshadowedOutgoingLuminance = 1e-05.xxx; // Avoid NaNs later
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
    float3x3 tangentToWorld = RotationMatrix3x3(gBuffer.Normal);
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

float3 EvaluateStochasticLighting(Light light, LTCTerms ltcTerms, LTCAnalyticEvaluationResult directLightingEvaluationResult, LightSample lightSample, float3 surfacePosition)
{
    // PDF is expected to be negative for when the sample vector missed the light
    if (lightSample.PDF < 0.0)
    {
        return 0.xxx;
    }

    float3 surfaceToLightDir = normalize(lightSample.IntersectionPoint.xyz - surfacePosition);
    LTCStochasticEvaluationResult brdfRayLightingEvaluationResult = EvaluateLTCLightingForSampleVector(light, ltcTerms, surfaceToLightDir, directLightingEvaluationResult.DiffuseProbability);
    float misPDF = lerp(lightSample.PDF, brdfRayLightingEvaluationResult.PDF, directLightingEvaluationResult.BRDFProbability);

    // This test should almost never fail because we just importance sampled from the BRDF.
    // Only an all black BRDF or a precision issue could trigger this.
    return directLightingEvaluationResult.BRDFProbability > 0.0f ? 
        // Stochastic estimate of incident light, without shadow
        // Note: projection cosine dot(w_i, n) is baked into the LTC-based BRDF
        brdfRayLightingEvaluationResult.OutgoingLuminance / misPDF :
        0.xxx;
}

void ShadeWithSphericalLights(
    GBufferStandard gBuffer, 
    LTCTerms ltcTerms, 
    LightTablePartitionInfo lightPartitionInfo, 
    float4 blueNoise,
    float3 viewDirection, 
    float3 surfacePosition,
    inout ShadingResult shadingResult,
    inout RTCache cache)
{
    for (uint lightIdx = 0; lightIdx < lightPartitionInfo.SphericalLightsCount; ++lightIdx)
    {
        uint lightTableOffset = lightIdx + lightPartitionInfo.SphericalLightsOffset;

        Light light = LightTable[lightTableOffset];
        LightPoints lightPoints = ComputeLightPoints(light, surfacePosition);
        ShericalLightSamplingInputs samplingInputs = ComputeSphericalLightSamplingInputs(light, surfacePosition);
        LTCAnalyticEvaluationResult directLightingEvaluationResult = EvaluateDirectSphericalLighting(light, lightPoints, gBuffer, ltcTerms, viewDirection, surfacePosition);

        [unroll]
        for (uint rayIdx = 0; rayIdx < RaysPerLight; ++rayIdx)
        {
            float4 randomNumbers = RandomNumbersForLight(lightTableOffset, rayIdx, blueNoise);

            // Randomly pick specular or diffuse lobe based on diffuse probability
            bool isSpecular = randomNumbers.z > directLightingEvaluationResult.DiffuseProbability;
            float3x3 M = isSpecular ? ltcTerms.MSpecular : ltcTerms.MDiffuse;

            // Pick a light sampling vector based on probability of taking a vector from BRDF distribution
            // versus picking a direct vector to one of random points on the light's surface
            bool sampleBRDF = randomNumbers.w <= directLightingEvaluationResult.BRDFProbability;

            float3 sampleVector = sampleBRDF ?
                LTCSampleVector(M, randomNumbers.x, randomNumbers.y) :
                SphericalLightSampleVector(samplingInputs, randomNumbers.x, randomNumbers.y);

            LightSample lightSample = SampleSphericalLight(light, samplingInputs, sampleVector);

            //shadingResult.StochasticUnshadowedOutgoingLuminance += EvaluateStochasticLighting(light, ltcTerms, directLightingEvaluationResult, lightSample, surfacePosition);
            
            uint index = lightTableOffset * RaysPerLight + rayIdx;
            SetRTCacheProbabilities(cache, index, directLightingEvaluationResult.BRDFProbability, directLightingEvaluationResult.DiffuseProbability, lightSample.PDF);
            SetRTCacheLightIntersectinPoint(cache, index, lightSample.IntersectionPoint);
        }

        shadingResult.AnalyticUnshadowedOutgoingLuminance += directLightingEvaluationResult.OutgoingLuminance;
    }
}

void ShadeWithRectangularLights(
    GBufferStandard gBuffer, 
    LTCTerms ltcTerms,
    LightTablePartitionInfo lightPartitionInfo,
    float4 blueNoise,
    float3 viewDirection,
    float3 surfacePosition, 
    inout ShadingResult shadingResult, 
    inout RTCache cache)
{
    for (uint lightIdx = 0; lightIdx < lightPartitionInfo.RectangularLightsCount; ++lightIdx)
    {
        uint lightTableOffset = lightIdx + lightPartitionInfo.RectangularLightsOffset;

        Light light = LightTable[lightTableOffset];
        LightPoints lightPoints = ComputeLightPoints(light, surfacePosition);
        RectangularLightSamplingInputs samplingInputs = ComputeRectangularLightSamplingInputs(lightPoints, surfacePosition);
        LTCAnalyticEvaluationResult directLightingEvaluationResult = EvaluateDirectRectangularLighting(light, lightPoints, gBuffer, ltcTerms, viewDirection, surfacePosition);

        [unroll]
        for (uint rayIdx = 0; rayIdx < RaysPerLight; ++rayIdx)
        {
            float4 randomNumbers = RandomNumbersForLight(lightTableOffset, rayIdx, blueNoise);
            bool isSpecular = randomNumbers.z > directLightingEvaluationResult.DiffuseProbability;
            float3x3 M = isSpecular ? ltcTerms.MSpecular : ltcTerms.MDiffuse;
            bool sampleBRDF = randomNumbers.w <= directLightingEvaluationResult.BRDFProbability;

            float3 sampleVector = sampleBRDF ? 
                LTCSampleVector(M, randomNumbers.x, randomNumbers.y) :
                RectangularLightSampleVector(samplingInputs, randomNumbers.x, randomNumbers.y);

            LightSample lightSample = SampleRectangularLight(light, samplingInputs, lightPoints, sampleVector);
            //shadingResult.StochasticUnshadowedOutgoingLuminance += EvaluateStochasticLighting(light, ltcTerms, directLightingEvaluationResult, lightSample, surfacePosition);

            uint index = lightTableOffset * RaysPerLight + rayIdx;
            SetRTCacheProbabilities(cache, index, directLightingEvaluationResult.BRDFProbability, directLightingEvaluationResult.DiffuseProbability, lightSample.PDF);
            SetRTCacheLightIntersectinPoint(cache, index, lightSample.IntersectionPoint);
        }

        shadingResult.AnalyticUnshadowedOutgoingLuminance += directLightingEvaluationResult.OutgoingLuminance;
    }
}

float4 TraceShadows(RTCache cache, float3 surfacePosition, float2 uv)
{
    // Shadow values for hard coded maximum of 4 lights
    float4 shadowValues = 0.xxxx;

    static const float3 Points[4] = { float3(10, 10, 10), float3 (-100, 0.83, 44), float3 (22, -90, 10), float3 (-8, 3, 32) };

    [unroll]
    for (uint i = 0; i < 2 /*MaxSupportedLights * RaysPerLight*/; ++i)
    {
        // Zero luminance indicates there is no light with index I or
        // we actually calculated 0 luminance, in any case, ray tracing is not required
       /* if (all(cache.StochasticOutgoingLuminances[i] <= 0.0))
        {
            continue;
        }*/

        float3 lightIntersectionPoint = cache.LightIntersectionPoints[i];
        float3 lightToSurface = surfacePosition - lightIntersectionPoint;
        float vectorLength = length(lightToSurface);
        float tmax = vectorLength - 1e-03;
        float tmin = 1e-03;

        RayDesc dxrRay;
        dxrRay.Origin = lightIntersectionPoint;
        dxrRay.Direction = lightToSurface / vectorLength;
        dxrRay.TMin = tmin;
        dxrRay.TMax = tmax;

        ShadowRayPayload shadowPayload = { 0.0 };

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

        shadowValues[i] = shadowPayload.ShadowFactor;
    }

    return shadowValues;
}

void ShadowStochasticOutgoingLuminance(RTCache rtCache, float4 shadowValues, inout ShadingResult shadingResult)
{
    [unroll]
    for (uint i = 0; i < MaxSupportedLights * RaysPerLight; ++i)
    {
        shadingResult.StochasticShadowedOutgoingLuminance += rtCache.LightIntersectionPoints[i] * shadowValues[i];
    }
}

ShadingResult EvaluateStandardGBufferLighting(GBufferStandard gBuffer, float2 uv, uint2 pixelIndex, float depth)
{
    Texture2D blueNoiseTexture = Textures2D[PassDataCB.BlueNoiseTextureIndex];
    Material material = MaterialTable[gBuffer.MaterialIndex];
    LightTablePartitionInfo partitionInfo = DecompressLightPartitionInfo();

    float4 blueNoise = blueNoiseTexture.Load(uint3(pixelIndex % PassDataCB.BlueNoiseTextureSize, 0));
    float3 surfacePosition = ReconstructWorldPosition(depth, uv, FrameDataCB.CameraInverseView, FrameDataCB.CameraInverseProjection);
    float3 viewDirection = normalize(FrameDataCB.CameraPosition.xyz - surfacePosition);

    LTCTerms ltcTerms = FetchLTCTerms(gBuffer, material, viewDirection);
    RTCache cache = ZeroRTCache();
    ShadingResult shadingResult = ZeroShadingResult();

    ShadeWithSphericalLights(gBuffer, ltcTerms, partitionInfo, blueNoise, viewDirection, surfacePosition, shadingResult, cache);
    ShadeWithRectangularLights(gBuffer, ltcTerms, partitionInfo, blueNoise, viewDirection, surfacePosition, shadingResult, cache);

    // Shadow values for 4 hard coded lights
    float4 shadowValues = TraceShadows(cache, surfacePosition, uv);
    ShadowStochasticOutgoingLuminance(cache, shadowValues, shadingResult);

    return shadingResult;
}

ShadingResult EvaluateEmissiveGBufferLighting(GBufferEmissive gBuffer)
{
    ShadingResult result = ZeroShadingResult();
    result.AnalyticUnshadowedOutgoingLuminance = gBuffer.LuminousIntensity;
    result.StochasticShadowedOutgoingLuminance = gBuffer.LuminousIntensity;
    result.StochasticUnshadowedOutgoingLuminance = gBuffer.LuminousIntensity;
    return result;
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

[shader("miss")]
void RayMiss(inout ShadowRayPayload payload)
{
    payload.ShadowFactor = 1.0;
}

struct CallableParam
{
    float TestData;
};

[shader("callable")]
void TestFunc(inout CallableParam parameter)
{
    //param.TestData = float3(0.0, 100.0, 0.0);
}

[shader("raygeneration")]
void RayGeneration()
{
    Texture2D<uint4> materialData = UInt4_Textures2D[PassDataCB.GBufferMaterialDataTextureIndex];
    Texture2D depthTexture = Textures2D[PassDataCB.GBufferDepthTextureIndex];
    RWTexture2D<float4> analyticOutputImage = RW_Float4_Textures2D[PassDataCB.AnalyticOutputTextureIndex];
    RWTexture2D<float4> stochasticUnshadowedOutputImage = RW_Float4_Textures2D[PassDataCB.StochasticUnshadowedOutputTextureIndex];
    RWTexture2D<float4> stochasticShadowedOutputImage = RW_Float4_Textures2D[PassDataCB.StochasticShadowedOutputTextureIndex];

    uint2 pixelIndex = DispatchRaysIndex().xy;
    float2 currenPixelLocation = pixelIndex + float2(0.5f, 0.5f);
    float2 pixelCenterUV = currenPixelLocation / DispatchRaysDimensions().xy;
    float depth = depthTexture.Load(uint3(pixelIndex, 0)).r;

    // Skip empty and emissive areas
    if (depth >= 1.0)
    {
        analyticOutputImage[pixelIndex].rgb = 0.xxx;
        return;
    } 

    GBufferEncoded encodedGBuffer;
    encodedGBuffer.MaterialData = materialData.Load(uint3(pixelIndex, 0));

    uint gBufferType = DecodeGBufferType(encodedGBuffer);
    ShadingResult shadingResult = ZeroShadingResult();

    switch (gBufferType)
    {
    case GBufferTypeStandard:
        shadingResult = EvaluateStandardGBufferLighting(DecodeGBufferStandard(encodedGBuffer), pixelCenterUV, pixelIndex, depth);
        break;

    case GBufferTypeEmissive:
        shadingResult = EvaluateEmissiveGBufferLighting(DecodeGBufferEmissive(encodedGBuffer));
        break;
    }

    ExposureOutput output = ExposeOutgoingLuminance(shadingResult.StochasticShadowedOutgoingLuminance);

    analyticOutputImage[pixelIndex] = float4(output.ExposedLuminance, 1.0);
    stochasticUnshadowedOutputImage[pixelIndex] = float4(shadingResult.StochasticUnshadowedOutgoingLuminance, 1.0);
    stochasticShadowedOutputImage[pixelIndex] = float4(shadingResult.StochasticShadowedOutgoingLuminance, 1.0);
}

#endif