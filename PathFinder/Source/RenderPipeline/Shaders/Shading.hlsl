#ifndef _Shading__
#define _Shading__

#include "Exposure.hlsl"
#include "Mesh.hlsl"
#include "GBuffer.hlsl"
#include "ShadingPacking.hlsl"

struct ShadingResult
{
    float3 AnalyticUnshadowedOutgoingLuminance;
    float3 StochasticUnshadowedOutgoingLuminance;
    float3 StochasticShadowedOutgoingLuminance;
};

struct ShadowRayPayload
{
    float ShadowFactor;
};

struct PassData
{
    GBufferTextureIndices GBufferIndices;
    // 1 4-component set of Halton numbers for rays of each light
    float4 HaltonSequence[MaxSupportedLights * RaysPerLight];
    // 16 byte boundary
    uint BlueNoiseTexIdx;
    uint AnalyticOutputTexIdx;
    uint StochasticShadowedOutputTexIdx;
    uint StochasticUnshadowedOutputTexIdx;
    // 16 byte boundary
    uint2 BlueNoiseTextureSize;
    uint __Pad0;
    uint __Pad1;
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
    static const uint sphericalLightsOffset = (compressed >> 24) & 0xFF;
    static const uint rectangularLightsOffset = (compressed >> 16) & 0xFF;
    static const uint ellipticalLightsOffset = (compressed >> 8) & 0xFF;
    static const uint totalLightsCount = compressed & 0xFF;

    static const uint sphericalLightsCount = rectangularLightsOffset - sphericalLightsOffset;
    static const uint rectangularLightsCount = ellipticalLightsOffset - rectangularLightsOffset;
    static const uint ellipticalLightsCount = totalLightsCount - rectangularLightsCount - sphericalLightsCount;

    LightTablePartitionInfo info;
    info.EllipticalLightsCount = ellipticalLightsCount;
    info.EllipticalLightsOffset = ellipticalLightsOffset;
    info.RectangularLightsCount = rectangularLightsCount;
    info.RectangularLightsOffset = rectangularLightsOffset;
    info.SphericalLightsCount = sphericalLightsCount;
    info.SphericalLightsOffset = sphericalLightsOffset;
    info.TotalLightsCount = totalLightsCount;

    return info;
}

ShadingResult ZeroShadingResult()
{
    ShadingResult result;
    result.AnalyticUnshadowedOutgoingLuminance = 0;
    result.StochasticShadowedOutgoingLuminance = 0;
    result.StochasticUnshadowedOutgoingLuminance = 0;
    return result;
}

//--------------------------------------------------------------------------------------------------

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

    // Rotate area light in (T1, T2, N) basis
    float3 T1, T2;
    T1 = normalize(viewDirWS - gBuffer.Normal * dot(viewDirWS, gBuffer.Normal));
    T2 = cross(gBuffer.Normal, T1);
    float3x3 Rinv = Matrix3x3ColumnMajor(T1, T2, gBuffer.Normal);
    float3x3 R = transpose(Rinv);

    LTCTerms terms;
    // This order of matrix multiplication is intended
    terms.MInvSpecular = mul(MInvSpecular, R);
    terms.MInvDiffuse = mul(MInvDiffuse, R);
    terms.MSpecular = mul(Rinv, MSpecular); // inv(AB) = inv(B)*inv(A)
    terms.MDiffuse = mul(Rinv, MDiffuse); // inv(AB) = inv(B)*inv(A)
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

float3 SampleBRDF(Light light, LTCTerms ltcTerms, LTCAnalyticEvaluationResult directLightingEvaluationResult, LightSample lightSample, float3 surfacePosition)
{
    // PDF is expected to be negative when the sample vector missed the light
    if (lightSample.PDF < 0.0)
    {
        return 0.xxx;
    }

    float3 surfaceToLightDir = normalize(lightSample.IntersectionPoint.xyz - surfacePosition);
    LTCSample brdfRayLightingEvaluationResult = SampleLTC(light, ltcTerms, surfaceToLightDir, directLightingEvaluationResult.DiffuseProbability);
    float misPDF = lerp(lightSample.PDF, brdfRayLightingEvaluationResult.PDF, directLightingEvaluationResult.BRDFProbability);

    // This test should almost never fail because we just importance sampled from the BRDF.
    // Only an all black BRDF or a precision issue could trigger this.
    return directLightingEvaluationResult.BRDFProbability > 0.0f ?
        // Stochastic estimate of incident light, without shadow
        // Note: projection cosine dot(w_i, n) is baked into the LTC-based BRDF
        brdfRayLightingEvaluationResult.BRDFMagnitude / misPDF :
        0.xxx; 
}

void ShadeWithSphericalLights(
    GBufferStandard gBuffer,
    LTCTerms ltcTerms,
    LightTablePartitionInfo lightPartitionInfo,
    float4 blueNoise,
    float3 viewDirection,
    float3 surfacePosition,
    inout RTData rtData,
    inout ShadingResult shadingResult)
{
    for (uint lightIdx = 0; lightIdx < lightPartitionInfo.SphericalLightsCount; ++lightIdx)
    {
        uint lightTableOffset = lightIdx + lightPartitionInfo.SphericalLightsOffset;

        Light light = LightTable[lightTableOffset];

        LightPoints lightPoints = ComputeLightPoints(light, surfacePosition);
        ShereLightSolidAngleSamplingInputs samplingInputs = ComputeSphericalLightSamplingInputs(light, surfacePosition);
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

            float3 brdf = SampleBRDF(light, ltcTerms, directLightingEvaluationResult, lightSample, surfacePosition) * RaysPerLightInverse;

            uint rayLightPairIndex = lightTableOffset * RaysPerLight + rayIdx;
            SetStochasticBRDFMagnitude(rtData, brdf, rayLightPairIndex);
            SetRaySphericalLightIntersectionPoint(rtData, light, lightSample.IntersectionPoint, rayLightPairIndex);
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
    inout RTData rtData,
    inout ShadingResult shadingResult)
{
    for (uint lightIdx = 0; lightIdx < lightPartitionInfo.RectangularLightsCount; ++lightIdx)
    {
        uint lightTableOffset = lightIdx + lightPartitionInfo.RectangularLightsOffset;

        Light light = LightTable[lightTableOffset];

        LightPoints lightPoints = ComputeLightPoints(light, surfacePosition);
        RectLightSolidAngleSamplingInputs samplingInputs = ComputeRectLightSolidAngleSamplingInputs(lightPoints, surfacePosition);
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
            float3 brdf = SampleBRDF(light, ltcTerms, directLightingEvaluationResult, lightSample, surfacePosition) * RaysPerLightInverse;

            uint rayLightPairIndex = lightTableOffset * RaysPerLight + rayIdx;
            SetStochasticBRDFMagnitude(rtData, brdf, rayLightPairIndex);
            SetRayRectangularLightIntersectionPoint(rtData, light, lightPoints.LightRotation, lightSample.IntersectionPoint, rayLightPairIndex);
        }

        shadingResult.AnalyticUnshadowedOutgoingLuminance += directLightingEvaluationResult.OutgoingLuminance;
    }
}

void ShadeWithEllipticalLights(
    GBufferStandard gBuffer,
    LTCTerms ltcTerms,
    LightTablePartitionInfo lightPartitionInfo,
    float4 blueNoise,
    float3 viewDirection,
    float3 surfacePosition,
    inout RTData rtData,
    inout ShadingResult shadingResult)
{
    for (uint lightIdx = 0; lightIdx < lightPartitionInfo.EllipticalLightsCount; ++lightIdx)
    {
        uint lightTableOffset = lightIdx + lightPartitionInfo.EllipticalLightsOffset;

        Light light = LightTable[lightTableOffset];

        // Treat elliptical light as rectangular because solid angle sampling of spherical ellipsoids 
        // is long, branch heavy and not very suited for real-time application, IMO.
        // Treating ellipse as rectangle means we will have some of the rays miss the light,
        // leading to a slightly increased variance, but it still will be better than area sampling.
        LightPoints lightPoints = ComputeLightPoints(light, surfacePosition);
        RectLightSolidAngleSamplingInputs samplingInputs = ComputeRectLightSolidAngleSamplingInputs(lightPoints, surfacePosition);
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

            LightSample lightSample = SampleEllipticalLight(light, samplingInputs, lightPoints, sampleVector);
            float3 brdf = SampleBRDF(light, ltcTerms, directLightingEvaluationResult, lightSample, surfacePosition) * RaysPerLightInverse;

            uint rayLightPairIndex = lightTableOffset * RaysPerLight + rayIdx;
            SetStochasticBRDFMagnitude(rtData, brdf, rayLightPairIndex);
            SetRayRectangularLightIntersectionPoint(rtData, light, lightPoints.LightRotation, lightSample.IntersectionPoint, rayLightPairIndex);
        }

        shadingResult.AnalyticUnshadowedOutgoingLuminance += directLightingEvaluationResult.OutgoingLuminance;
    }
}

float4 TraceShadows(RTData rtData, float3 surfacePosition, float4 blueNoise)
{
    // Shadow values for hard coded maximum of 4 lights
    float4 shadowValues = 0.xxxx;
    
    [unroll]
    for (uint i = 0; i < MaxSupportedLights * RaysPerLight; ++i)
    {
        if (rtData.BRDFResponses[i] == 0)
        {
            continue;
        }

        uint lightIndex = i * RaysPerLightInverse;
        Light light = LightTable[lightIndex];
        float3 lightIntersectionPoint = 0.xxx;
        float3x3 lightRotation = RotationMatrix3x3(light.Orientation.xyz);

        [branch] switch (light.LightType)
        {
        case LightTypeSphere:
            lightIntersectionPoint = GetRaySphericalLightIntersectionPoint(rtData, light, i);
            break;
        case LightTypeRectangle:
            lightIntersectionPoint = GetRayRectangularLightIntersectionPoint(rtData, light, lightRotation, i);
            break;
        case LightTypeEllipse:
            lightIntersectionPoint = GetRayDiskLightIntersectionPoint(rtData, light, lightRotation, i);
            break;
        }

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

void CombineStochasticLightingAndShadows(RTData rtData, inout ShadingResult shadingResult)
{
    [unroll]
    for (uint i = 0; i < MaxSupportedLights * RaysPerLight; ++i)
    {
        if (rtData.BRDFResponses[i] == 0)
        {
            continue;
        }

        uint lightIndex = i * RaysPerLightInverse;
        Light light = LightTable[lightIndex];
        float3 brdf = GetStochasticBRDFMagnitude(rtData, i);
        float3 unshadowed = brdf * light.Color.rgb * light.Luminance;

        shadingResult.StochasticUnshadowedOutgoingLuminance += unshadowed;
        shadingResult.StochasticShadowedOutgoingLuminance += unshadowed * rtData.ShadowFactors[i];
    }
}

ShadingResult EvaluateStandardGBufferLighting(GBufferTexturePack gBufferTextures, float2 uv, uint2 pixelIndex, float depth)
{
    Texture2D blueNoiseTexture = Textures2D[PassDataCB.BlueNoiseTexIdx];

    GBufferStandard gBuffer;
    LoadStandardGBuffer(gBuffer, gBufferTextures, pixelIndex);

    Material material = MaterialTable[gBuffer.MaterialIndex];
    LightTablePartitionInfo partitionInfo = DecompressLightPartitionInfo();

    float4 blueNoise = blueNoiseTexture.Load(uint3(pixelIndex % PassDataCB.BlueNoiseTextureSize, 0));
    float3 surfacePosition = ReconstructWorldSpacePosition(depth, uv, FrameDataCB.CurrentFrameCamera);
    float3 viewDirection = normalize(FrameDataCB.CurrentFrameCamera.Position.xyz - surfacePosition);

    LTCTerms ltcTerms = FetchLTCTerms(gBuffer, material, viewDirection);
    ShadingResult shadingResult = ZeroShadingResult();
    RTData rtData = ZeroRTData();

    ShadeWithSphericalLights(gBuffer, ltcTerms, partitionInfo, blueNoise, viewDirection, surfacePosition, rtData, shadingResult);
    ShadeWithRectangularLights(gBuffer, ltcTerms, partitionInfo, blueNoise, viewDirection, surfacePosition, rtData, shadingResult);
    ShadeWithEllipticalLights(gBuffer, ltcTerms, partitionInfo, blueNoise, viewDirection, surfacePosition, rtData, shadingResult);

    rtData.ShadowFactors = TraceShadows(rtData, surfacePosition, blueNoise);

    CombineStochasticLightingAndShadows(rtData, shadingResult);

    return shadingResult;
}

ShadingResult EvaluateEmissiveGBufferLighting(GBufferTexturePack gBufferTextures, uint2 pixelIndex)
{
    GBufferEmissive gBuffer;
    LoadEmissiveGBuffer(gBuffer, gBufferTextures, pixelIndex);

    Light light = LightTable[gBuffer.LightIndex];

    ShadingResult result = ZeroShadingResult();
    result.AnalyticUnshadowedOutgoingLuminance = light.Luminance * light.Color.rgb;
    result.StochasticShadowedOutgoingLuminance = 0;
    result.StochasticUnshadowedOutgoingLuminance = 0;
    return result;
}

[shader("miss")]
void RayMiss(inout ShadowRayPayload payload)
{
    payload.ShadowFactor = 1.0;
}

void OutputShadingResult(ShadingResult shadingResult, uint2 pixelIndex)
{
    RWTexture2D<float4> analyticOutput = RW_Float4_Textures2D[PassDataCB.AnalyticOutputTexIdx];
    RWTexture2D<float4> stochasticUnshadowedOutput = RW_Float4_Textures2D[PassDataCB.StochasticUnshadowedOutputTexIdx];
    RWTexture2D<float4> stochasticShadowedOutput = RW_Float4_Textures2D[PassDataCB.StochasticShadowedOutputTexIdx];

    shadingResult.AnalyticUnshadowedOutgoingLuminance = ExposeLuminance(shadingResult.AnalyticUnshadowedOutgoingLuminance, FrameDataCB.CurrentFrameCamera);
    shadingResult.StochasticShadowedOutgoingLuminance = ExposeLuminance(shadingResult.StochasticShadowedOutgoingLuminance, FrameDataCB.CurrentFrameCamera);
    shadingResult.StochasticUnshadowedOutgoingLuminance = ExposeLuminance(shadingResult.StochasticUnshadowedOutgoingLuminance, FrameDataCB.CurrentFrameCamera);

    analyticOutput[pixelIndex] = float4(shadingResult.AnalyticUnshadowedOutgoingLuminance, 1.0);
    stochasticShadowedOutput[pixelIndex] = float4(shadingResult.StochasticShadowedOutgoingLuminance, 1.0);
    stochasticUnshadowedOutput[pixelIndex] = float4(shadingResult.StochasticUnshadowedOutgoingLuminance, 1.0);
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
    float2 currenPixelLocation = pixelIndex + float2(0.5f, 0.5f);
    float2 pixelCenterUV = currenPixelLocation / DispatchRaysDimensions().xy;

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
        shadingResult = EvaluateStandardGBufferLighting(gBufferTextures, pixelCenterUV, pixelIndex, depth);
        break;

    case GBufferTypeEmissive:
        shadingResult = EvaluateEmissiveGBufferLighting(gBufferTextures, pixelIndex);
        break;
    }

    OutputShadingResult(shadingResult, pixelIndex);
}

#endif