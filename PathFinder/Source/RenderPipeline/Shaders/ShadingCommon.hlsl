#ifndef _ShadingCommon__
#define _ShadingCommon__

#include "Mesh.hlsl"
#include "Vertices.hlsl"
#include "GBuffer.hlsl"
#include "Random.hlsl"
#include "ColorConversion.hlsl"
#include "Light.hlsl"
#include "Packing.hlsl"
#include "Utils.hlsl"

#include "MandatoryEntryPointInclude.hlsl"

struct RootConstants
{
    uint CompressedLightPartitionInfo;
};

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);
RaytracingAccelerationStructure SceneBVH : register(t0);
StructuredBuffer<Light> LightTable : register(t1);
StructuredBuffer<Material> MaterialTable : register(t2);
StructuredBuffer<Vertex1P1N1UV1T1BT> UnifiedVertexBuffer : register(t3);
StructuredBuffer<uint> UnifiedIndexBuffer : register(t4);
StructuredBuffer<MeshInstance> MeshInstanceTable : register(t5);

struct RTData
{
    // 4 components for 4 ray-light pairs
    float4x3 BRDFResponses;
    uint4 RayLightIntersectionData;
    float4 ShadowFactors;
};

struct ShadingResult
{
#ifndef SHADING_STOCHASTIC_ONLY
    float3 AnalyticUnshadowedOutgoingLuminance;
    float3 StochasticUnshadowedOutgoingLuminance;
#endif // !SHADING_STOCHASTIC_ONLY

    float3 StochasticShadowedOutgoingLuminance;
};

struct RandomSequences
{
    float4 BlueNoise;
    float4 Halton;
};

// Turing-level hardware can realistically work with 4 lights and 1 ray per light, per tile.
// We should not bother with more to make space for other rendering workloads.
static const uint TotalMaxRayCount = 4;

RTData ZeroRTData()
{
    RTData data;
    data.BRDFResponses = 0.0;
    data.RayLightIntersectionData = 0.0;
    data.ShadowFactors = 1.0;
    return data;
}

//--------------------------------------------------------------------------------------------------
// Functions to pack and unpack data necessary for shadows ray tracing pass.
// Avoids using arrays and allows us to pack everything into single 4-component registers to avoid 
// register spilling.
//--------------------------------------------------------------------------------------------------
void SetStochasticBRDFMagnitude(inout RTData rtData, float3 brdf, uint rayLightPairIndex)
{
    rtData.BRDFResponses[rayLightPairIndex].rgb = brdf;
}

float3 GetStochasticBRDFMagnitude(RTData rtData, uint rayLightPairIndex)
{
    return rtData.BRDFResponses[rayLightPairIndex].rgb;
}

bool IsStochasticBRDFMagnitudeNonZero(RTData rtData, uint rayLightPairIndex)
{
    return any(rtData.BRDFResponses[rayLightPairIndex]) > 0.0;
}

void SetRaySphericalLightIntersectionPoint(inout RTData rtData, Light light, float3 interectionPoint, uint rayLightPairIndex)
{
    // For spherical lights we encode a normalized direction from light's center to intersection point
    float3 lightToIntersection = normalize(interectionPoint - light.Position.xyz);

    rtData.RayLightIntersectionData[rayLightPairIndex] = OctEncodePack(lightToIntersection);
}

float3 GetRaySphericalLightIntersectionPoint(RTData rtData, Light light, uint rayLightPairIndex)
{
    float3 lightToIntersection = OctUnpackDecode(rtData.RayLightIntersectionData[rayLightPairIndex]);
    return lightToIntersection * light.Width * 0.5 + light.Position.xyz;
}

void SetRayRectangularLightIntersectionPoint(inout RTData rtData, Light light, float3x3 lightRotation, float3 interectionPoint, uint rayLightPairIndex)
{
    // Translate point to light's local coordinate system
    float3 localPoint = interectionPoint - light.Position.xyz;
    // Rotate point back so it's positioned on a rectangle perpendicular to Z axis
    float3 zAlighnedPoint = mul(transpose(lightRotation), localPoint);
    // Normalize xy to get uv of the intersection point
    float2 uv = zAlighnedPoint.xy / float2(light.Width, light.Height) + 0.5;

    rtData.RayLightIntersectionData[rayLightPairIndex] = PackUnorm2x16(uv.x, uv.y, 1.0);
}

float3 GetRayRectangularLightIntersectionPoint(RTData rtData, Light light, float3x3 lightRotation, uint rayLightPairIndex)
{
    float2 uv = UnpackUnorm2x16(rtData.RayLightIntersectionData[rayLightPairIndex], 1.0);
    float2 localXY = (uv - 0.5) * float2(light.Width, light.Height);
    float3 localRotated = mul(lightRotation, float3(localXY, 0.0));
    float3 worldPoint = localRotated + light.Position.xyz;

    return worldPoint;
}

void SetRayDiskLightIntersectionPoint(inout RTData rtData, Light light, float3x3 lightRotation, float3 interectionPoint, uint rayLightPairIndex)
{
    // Code for rect light can be reused here
    SetRayRectangularLightIntersectionPoint(rtData, light, lightRotation, interectionPoint, rayLightPairIndex);
}

float3 GetRayDiskLightIntersectionPoint(RTData rtData, Light light, float3x3 lightRotation, uint rayLightPairIndex)
{
    return GetRayRectangularLightIntersectionPoint(rtData, light, lightRotation, rayLightPairIndex);
}

LightTablePartitionInfo DecompressLightPartitionInfo()
{
    // Lights are expected to be placed in order: spherical -> rectangular -> disk
    uint compressed = RootConstantBuffer.CompressedLightPartitionInfo;

    static const uint sphericalLightsCount = (compressed >> 24) & 0xFF;
    static const uint rectangularLightsCount = (compressed >> 16) & 0xFF;
    static const uint ellipticalLightsCount = (compressed >> 8) & 0xFF;
    static const uint totalLightsCount = sphericalLightsCount + rectangularLightsCount + ellipticalLightsCount;

    LightTablePartitionInfo info;

    info.SphericalLightsCount = sphericalLightsCount;
    info.SphericalLightsOffset = 0;

    info.RectangularLightsCount = rectangularLightsCount;
    info.RectangularLightsOffset = sphericalLightsCount;

    info.EllipticalLightsCount = ellipticalLightsCount;
    info.EllipticalLightsOffset = sphericalLightsCount + rectangularLightsCount;

    info.TotalLightsCount = totalLightsCount;

    return info;
}

ShadingResult ZeroShadingResult()
{
    ShadingResult result;
    result.StochasticShadowedOutgoingLuminance = 0;

#ifndef SHADING_STOCHASTIC_ONLY
    result.StochasticUnshadowedOutgoingLuminance = 0;
    result.AnalyticUnshadowedOutgoingLuminance = 0;
#endif

    return result;
}

// Distributes rays between lights depending on their total amount
uint RaysPerLight(LightTablePartitionInfo lightTableInfo)
{
    uint raysPerLight = TotalMaxRayCount / lightTableInfo.TotalLightsCount;
    return raysPerLight;
}

float4 RandomNumbersForLight(RandomSequences randomSequences, uint rayIndex)
{
    // Used for 2D position on light/BRDF
    float u1 = frac(randomSequences.Halton.r + randomSequences.BlueNoise.r);
    float u2 = frac(randomSequences.Halton.g + randomSequences.BlueNoise.g);

    // Choosing BRDF lobes
    float u3 = frac(randomSequences.Halton.b + randomSequences.BlueNoise.b);

    // Choosing between light and BRDF sampling
    float u4 = frac(randomSequences.Halton.a + randomSequences.BlueNoise.a);

    return float4(u1, u2, u3, u4);
}

//--------------------------------------------------------------------------------------------------
// An implementation of Combining Analytic Direct Illumination and Stochastic Shadows
// http://casual-effects.com/research/Heitz2018Shadow/Heitz2018SIGGRAPHTalk.pdf
// https://hal.archives-ouvertes.fr/hal-01761558/file/heitzI3D2018_slides.pdf
// https://research.nvidia.com/sites/default/files/pubs/2018-05_Combining-Analytic-Direct//I3D2018_combining.pdf

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

    float4 inverseMatrixComponentsSpecular = LTC_LUT_Specular_MatrixInverse.SampleLevel(LinearClampSampler(), uv, 0);
    float4 matrixComponentsSpecular = LTC_LUT_Specular_Matrix.SampleLevel(LinearClampSampler(), uv, 0);
    float4 maskingFresnelAndScaleSpecular = LTC_LUT_Specular_Terms.SampleLevel(LinearClampSampler(), uv, 0);

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

    float4 inverseMatrixComponentsDiffuse = LTC_LUT_Diffuse_MatrixInverse.SampleLevel(LinearClampSampler(), uv, 0);
    float4 matrixComponentsDiffuse = LTC_LUT_Diffuse_Matrix.SampleLevel(LinearClampSampler(), uv, 0);

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
    terms.LUTSampler = LinearClampSampler();

    return terms;
}

float3 SampleBRDF(LTCTerms ltcTerms, LTCAnalyticEvaluationResult directLightingEvaluationResult, LightSample lightSample, float3 surfacePosition)
{
    // PDF is expected to be negative when the sample vector missed the light
    if (lightSample.PDF < 0.0)
        return 0.0;

    float3 surfaceToLightDir = normalize(lightSample.IntersectionPoint.xyz - surfacePosition);
    LTCSample brdfRayLightingEvaluationResult = SampleLTC(ltcTerms, surfaceToLightDir, directLightingEvaluationResult.DiffuseProbability);
    float misPDF = lerp(lightSample.PDF, brdfRayLightingEvaluationResult.PDF, directLightingEvaluationResult.BRDFProbability);

    // This test should almost never fail because we just importance sampled from the BRDF.
    // Only an all black BRDF or a precision issue could trigger this.
    //
    // Stochastic estimate of incident light, without shadow
    // Note: projection cosine dot(w_i, n) is baked into the LTC-based BRDF
    float3 brdf = directLightingEvaluationResult.BRDFProbability > 0.0f ? 
        brdfRayLightingEvaluationResult.BRDFMagnitude / max(misPDF, 0.0001) : 0.0; 

    return brdf;
}

void ShadeWithSphericalLights(
    GBufferStandard gBuffer,
    LTCTerms ltcTerms,
    LightTablePartitionInfo lightPartitionInfo,
    RandomSequences randomSequences,
    float3 viewDirection,
    float3 surfacePosition,
    inout RTData rtData,
    inout ShadingResult shadingResult)
{
    uint raysPerLight = RaysPerLight(lightPartitionInfo);

    for (uint lightIdx = 0; lightIdx < lightPartitionInfo.SphericalLightsCount; ++lightIdx)
    {
        uint lightTableOffset = lightIdx + lightPartitionInfo.SphericalLightsOffset;

        Light light = LightTable[lightTableOffset];

        LightPoints lightPoints = ComputeLightPoints(light, surfacePosition);
        ShereLightSolidAngleSamplingInputs samplingInputs = ComputeSphericalLightSamplingInputs(light, surfacePosition);
        LTCAnalyticEvaluationResult directLightingEvaluationResult = EvaluateDirectSphericalLighting(light, lightPoints, gBuffer, ltcTerms, viewDirection, surfacePosition);

        [unroll]
        for (uint rayIdx = 0; rayIdx < raysPerLight; ++rayIdx)
        {
            float4 randomNumbers = RandomNumbersForLight(randomSequences, rayIdx);

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

            float3 brdf = SampleBRDF(ltcTerms, directLightingEvaluationResult, lightSample, surfacePosition) / raysPerLight;

            uint rayLightPairIndex = lightTableOffset * raysPerLight + rayIdx;
            SetStochasticBRDFMagnitude(rtData, brdf, rayLightPairIndex);
            SetRaySphericalLightIntersectionPoint(rtData, light, lightSample.IntersectionPoint, rayLightPairIndex);
        }

#ifndef SHADING_STOCHASTIC_ONLY
        shadingResult.AnalyticUnshadowedOutgoingLuminance += directLightingEvaluationResult.OutgoingLuminance;
#endif
    }
}

void ShadeWithRectangularLights(
    GBufferStandard gBuffer,
    LTCTerms ltcTerms,
    LightTablePartitionInfo lightPartitionInfo,
    RandomSequences randomSequences,
    float3 viewDirection,
    float3 surfacePosition,
    inout RTData rtData,
    inout ShadingResult shadingResult)
{
    uint raysPerLight = RaysPerLight(lightPartitionInfo);

    for (uint lightIdx = 0; lightIdx < lightPartitionInfo.RectangularLightsCount; ++lightIdx)
    {
        uint lightTableOffset = lightIdx + lightPartitionInfo.RectangularLightsOffset;

        Light light = LightTable[lightTableOffset];

        LightPoints lightPoints = ComputeLightPoints(light, surfacePosition);
        RectLightSolidAngleSamplingInputs samplingInputs = ComputeRectLightSolidAngleSamplingInputs(lightPoints, surfacePosition);
        LTCAnalyticEvaluationResult directLightingEvaluationResult = EvaluateDirectRectangularLighting(light, lightPoints, gBuffer, ltcTerms, viewDirection, surfacePosition);

        [unroll]
        for (uint rayIdx = 0; rayIdx < raysPerLight; ++rayIdx)
        {
            float4 randomNumbers = RandomNumbersForLight(randomSequences, rayIdx);
            bool isSpecular = randomNumbers.z > directLightingEvaluationResult.DiffuseProbability;
            float3x3 M = isSpecular ? ltcTerms.MSpecular : ltcTerms.MDiffuse;
            bool sampleBRDF = randomNumbers.w <= directLightingEvaluationResult.BRDFProbability;

            float3 sampleVector = sampleBRDF ?
                LTCSampleVector(M, randomNumbers.x, randomNumbers.y) :
                RectangularLightSampleVector(samplingInputs, randomNumbers.x, randomNumbers.y);

            LightSample lightSample = SampleRectangularLight(light, samplingInputs, lightPoints, sampleVector);
            float3 brdf = SampleBRDF(ltcTerms, directLightingEvaluationResult, lightSample, surfacePosition) / raysPerLight;

            uint rayLightPairIndex = lightTableOffset * raysPerLight + rayIdx;
            SetStochasticBRDFMagnitude(rtData, brdf, rayLightPairIndex);
            SetRayRectangularLightIntersectionPoint(rtData, light, lightPoints.LightRotation, lightSample.IntersectionPoint, rayLightPairIndex);
        }

#ifndef SHADING_STOCHASTIC_ONLY
        shadingResult.AnalyticUnshadowedOutgoingLuminance += directLightingEvaluationResult.OutgoingLuminance;
#endif
    }
}

void ShadeWithEllipticalLights(
    GBufferStandard gBuffer,
    LTCTerms ltcTerms,
    LightTablePartitionInfo lightPartitionInfo,
    RandomSequences randomSequences,
    float3 viewDirection,
    float3 surfacePosition,
    inout RTData rtData,
    inout ShadingResult shadingResult)
{
    uint raysPerLight = RaysPerLight(lightPartitionInfo);

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
        for (uint rayIdx = 0; rayIdx < raysPerLight; ++rayIdx)
        {
            float4 randomNumbers = RandomNumbersForLight(randomSequences, rayIdx);
            bool isSpecular = randomNumbers.z > directLightingEvaluationResult.DiffuseProbability;
            float3x3 M = isSpecular ? ltcTerms.MSpecular : ltcTerms.MDiffuse;
            bool sampleBRDF = randomNumbers.w <= directLightingEvaluationResult.BRDFProbability;

            float3 sampleVector = sampleBRDF ?
                LTCSampleVector(M, randomNumbers.x, randomNumbers.y) :
                RectangularLightSampleVector(samplingInputs, randomNumbers.x, randomNumbers.y);

            LightSample lightSample = SampleEllipticalLight(light, samplingInputs, lightPoints, sampleVector);
            float3 brdf = SampleBRDF(ltcTerms, directLightingEvaluationResult, lightSample, surfacePosition) / raysPerLight;

            uint rayLightPairIndex = lightTableOffset * raysPerLight + rayIdx;
            SetStochasticBRDFMagnitude(rtData, brdf, rayLightPairIndex);
            SetRayRectangularLightIntersectionPoint(rtData, light, lightPoints.LightRotation, lightSample.IntersectionPoint, rayLightPairIndex);
        }

#ifndef SHADING_STOCHASTIC_ONLY
        shadingResult.AnalyticUnshadowedOutgoingLuminance += directLightingEvaluationResult.OutgoingLuminance;
#endif
    }
}

float4 TraceShadows(RTData rtData, LightTablePartitionInfo lightPartitionInfo, float3 surfacePosition)
{
    // Shadow values for hard coded maximum of 4 lights
    float4 shadowValues = 0.0;
    uint raysPerLight = RaysPerLight(lightPartitionInfo);

    const uint RayFlags = 
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
        RAY_FLAG_FORCE_OPAQUE |            // Skip any hit shaders
        RAY_FLAG_SKIP_CLOSEST_HIT_SHADER; // Skip closest hit shaders,

    RayQuery<RayFlags> rayQuery;

    [unroll]
    for (uint i = 0; i < TotalMaxRayCount; ++i)
    {
        if (!IsStochasticBRDFMagnitudeNonZero(rtData, i))
        {
            continue;
        }

        uint lightIndex = i / raysPerLight;
        Light light = LightTable[lightIndex];
        float3 lightIntersectionPoint = 0.0;
        float3x3 lightRotation = RotationMatrix3x3(light.Orientation.xyz);

        [branch]
        switch (light.LightType)
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

        rayQuery.TraceRayInline(SceneBVH, RayFlags, EntityMaskMeshInstance, dxrRay);
        rayQuery.Proceed();

        shadowValues[i] = rayQuery.CommittedStatus() != COMMITTED_TRIANGLE_HIT ? 1.0 : 0.0;
    }

    return shadowValues;
}

void CombineStochasticLightingAndShadows(RTData rtData, LightTablePartitionInfo lightPartitionInfo, inout ShadingResult shadingResult)
{
    uint raysPerLight = RaysPerLight(lightPartitionInfo);

    [unroll]
    for (uint i = 0; i < TotalMaxRayCount; ++i)
    {
        if (!IsStochasticBRDFMagnitudeNonZero(rtData, i))
        {
            continue;
        }

        uint lightIndex = i / raysPerLight;
        Light light = LightTable[lightIndex];
        float3 brdf = GetStochasticBRDFMagnitude(rtData, i);
        float3 unshadowed = brdf * light.Color.rgb * light.Luminance;

        shadingResult.StochasticShadowedOutgoingLuminance += unshadowed * rtData.ShadowFactors[i];

#ifndef SHADING_STOCHASTIC_ONLY
        shadingResult.StochasticUnshadowedOutgoingLuminance += unshadowed;
#endif
    }
}

ShadingResult EvaluateStandardGBufferLighting(GBufferStandard gBuffer, Material material, float3 surfacePosition, float3 viewerPosition, RandomSequences randomSequences)
{
    LightTablePartitionInfo partitionInfo = DecompressLightPartitionInfo();

    float3 viewDirection = normalize(viewerPosition - surfacePosition);

    LTCTerms ltcTerms = FetchLTCTerms(gBuffer, material, viewDirection);
    ShadingResult shadingResult = ZeroShadingResult();
    RTData rtData = ZeroRTData();

    ShadeWithSphericalLights(gBuffer, ltcTerms, partitionInfo, randomSequences, viewDirection, surfacePosition, rtData, shadingResult);
    ShadeWithRectangularLights(gBuffer, ltcTerms, partitionInfo, randomSequences, viewDirection, surfacePosition, rtData, shadingResult);
    ShadeWithEllipticalLights(gBuffer, ltcTerms, partitionInfo, randomSequences, viewDirection, surfacePosition, rtData, shadingResult);

    rtData.ShadowFactors = TraceShadows(rtData, partitionInfo, surfacePosition);

    CombineStochasticLightingAndShadows(rtData, partitionInfo, shadingResult);

    return shadingResult;
}

#endif