#ifndef _Light__
#define _Light__

#include "Matrix.hlsl"
#include "Geometry.hlsl"
#include "Constants.hlsl"
#include "LTC.hlsl"
#include "Mesh.hlsl"
#include "GBuffer.hlsl"
#include "ColorConversion.hlsl"

static const uint LightTypeSphere = 0;
static const uint LightTypeRectangle = 1;
static const uint LightTypeEllipse = 2;

struct Light
{
    float4 Orientation;
    float4 Position;
    float4 Color;
    float Luminance;
    float Width;
    float Height;
    uint LightType;
    float4x4 ModelMatrix;
    uint UnifiedVertexBufferOffset;
    uint UnifiedIndexBufferOffset;
    uint IndexCount;
};

struct LightTablePartitionInfo
{
    uint SphericalLightsOffset;
    uint RectangularLightsOffset;
    uint EllipticalLightsOffset;
    uint SphericalLightsCount;
    uint RectangularLightsCount;
    uint EllipticalLightsCount;
    uint TotalLightsCount;
    uint Pad1__;
};

struct LTCTerms
{
    float3x3 MInvSpecular;
    float3x3 MInvDiffuse;
    float3x3 MSpecular;
    float3x3 MDiffuse;
    float MDetSpecular;
    float MDetDiffuse;
    float3 SurfaceSpecularAlbedo;
    float3 SurfaceDiffuseAlbedo;
    Texture2D LUT_Specular_Terms;
    Texture2D LUT_Diffuse_Terms;
    uint2 LUTSize;
    sampler LUTSampler;
};

struct LTCAnalyticEvaluationResult
{
    // Luminance outgoing from the surface point
    // in the view direction
    float3 OutgoingLuminance;

    // Probability of sampling BRDF
    float BRDFProbability;

    // Probability of sampling Diffuse Lobe
    // instead of Specular when BRDF sampling is chosen
    float DiffuseProbability;
};

struct LTCSample
{
    // Luminance outgoing from the surface point
    // in the view direction
    float3 BRDFMagnitude;

    // Probability density function value
    // for the sampling vector used to evaluate
    // LTC lighting
    float PDF;
};

struct LightPoints
{
    float3 Points[4];
    float3x3 LightRotation;
};

struct LightSample
{
    // Ray-light intersection point
    float3 IntersectionPoint;

    // Probability density function value
    // evaluated for surface-to-light vector.
    float PDF;
};

// Data required to sample rectangular light using solid angle sampling
struct RectLightSolidAngleSamplingInputs 
{
    float SolidAngle;
    float3 SurfacePoint;

    float3 x, y, z;
    float z0, z0sq;
    float x0, y0, y0sq;
    float x1, y1, y1sq;
    float b0, b1, b0sq, k;
};

struct RectLightAreaSamplingInputs
{
    float Area;
    float3 SurfacePoint;
    float3x3 LightPointLocalToWorldRotation;
};

struct DiskLightAreaSamplingInputs
{
    float Area;
    float3x3 LightPointLocalToWorldRotation;
};

// Data required to sample spherical light
struct ShereLightSolidAngleSamplingInputs
{
    float SolidAngle;
    float3 SurfacePoint;

    // A matrix to rotate a sampling vector from surface's
    // local space to world space
    float3x3 SampleLocalToWorldRotation;

    // Terms required for solid angle sampling
    float Q;
};

LightSample ZeroLightSample()
{
    LightSample lightSample;
    lightSample.IntersectionPoint = 0.xxx;
    lightSample.PDF = -1.0;
    return lightSample;
}

LTCAnalyticEvaluationResult ZeroLTCAnalyticEvaluationResult()
{
    LTCAnalyticEvaluationResult result;
    result.BRDFProbability = 0.0;
    result.DiffuseProbability = 0.0;
    result.OutgoingLuminance = 0.0;
    return result;
}

float LTCSampleVectorPDF(float3x3 MInv, float MDet, float3 L)
{
    float3 LCosine = mul(MInv, L);
    float l2 = dot(LCosine, LCosine);
    float Jacobian = MDet * l2 * l2;

    return max(LCosine.z, 0.0f) / max(Pi * Jacobian, 0.0001);
}

float3 LTCSampleVector(float3x3 M, float u1, float u2) 
{
    float ct = sqrt(u1);
    float st = sqrt(1.0f - u1);
    float phi = 2.0f * Pi * u2;

    float3 dir = float3(st * cos(phi), st * sin(phi), ct);

    float3 L = mul(M, dir);
    float3 w_i = normalize(L);

    return w_i;
}

LTCSample SampleLTC(LTCTerms ltcTerms, float3 sampleVector, float diffuseProbability) 
{
    float specularPDF = LTCSampleVectorPDF(ltcTerms.MInvSpecular, ltcTerms.MDetSpecular, sampleVector);
    float diffusePDF = LTCSampleVectorPDF(ltcTerms.MInvDiffuse, ltcTerms.MDetDiffuse, sampleVector);

    LTCSample result;
    result.PDF = lerp(specularPDF, diffusePDF, diffuseProbability);
    result.BRDFMagnitude = ltcTerms.SurfaceSpecularAlbedo * specularPDF + ltcTerms.SurfaceDiffuseAlbedo * diffusePDF;

    return result;
}

LTCAnalyticEvaluationResult ApplyMaterialAndLightingToLTCLobes(float specularLobe, float diffuseLobe, Light light, LTCTerms ltcTerms)
{
    float3 specular = specularLobe * ltcTerms.SurfaceSpecularAlbedo;
    float3 diffuse = diffuseLobe * ltcTerms.SurfaceDiffuseAlbedo;

    float specularMagnitude = CIELuminance(specular);
    float diffuseMagnitude = CIELuminance(diffuse);

    LTCAnalyticEvaluationResult evaluationResult;
    evaluationResult.OutgoingLuminance = (diffuse + specular) * light.Color.rgb * light.Luminance;
    evaluationResult.DiffuseProbability = diffuseMagnitude / max(diffuseMagnitude + specularMagnitude, 1e-5);
    evaluationResult.BRDFProbability = lerp(specularLobe, diffuseLobe, evaluationResult.DiffuseProbability);

    return evaluationResult;
}

LTCAnalyticEvaluationResult EvaluateDirectDiskLighting(
    Light light, 
    LightPoints lightPoints,
    GBufferStandard gBuffer,
    LTCTerms ltcTerms,
    float3 viewDirection,
    float3 surfacePosition)
{
    float specularLobe = LTCEvaluateDisk(gBuffer.Normal, viewDirection, surfacePosition, ltcTerms.MInvSpecular, lightPoints.Points, ltcTerms.LUT_Specular_Terms, ltcTerms.LUTSize, ltcTerms.LUTSampler);
    float diffuseLobe = LTCEvaluateDisk(gBuffer.Normal, viewDirection, surfacePosition, ltcTerms.MInvDiffuse, lightPoints.Points, ltcTerms.LUT_Diffuse_Terms, ltcTerms.LUTSize, ltcTerms.LUTSampler);

    return ApplyMaterialAndLightingToLTCLobes(specularLobe, diffuseLobe, light, ltcTerms);
}

LTCAnalyticEvaluationResult EvaluateDirectSphericalLighting(
    Light light,
    LightPoints lightPoints,
    GBufferStandard gBuffer,
    LTCTerms ltcTerms,
    float3 viewDirection,
    float3 surfacePosition)
{
    float specularLobe = LTCEvaluateDisk(gBuffer.Normal, viewDirection, surfacePosition, ltcTerms.MInvSpecular, lightPoints.Points, ltcTerms.LUT_Specular_Terms, ltcTerms.LUTSize, ltcTerms.LUTSampler);
    float diffuseLobe = LTCEvaluateDisk(gBuffer.Normal, viewDirection, surfacePosition, ltcTerms.MInvDiffuse, lightPoints.Points, ltcTerms.LUT_Diffuse_Terms, ltcTerms.LUTSize, ltcTerms.LUTSampler);

    return ApplyMaterialAndLightingToLTCLobes(specularLobe, diffuseLobe, light, ltcTerms);
}

LTCAnalyticEvaluationResult EvaluateDirectRectangularLighting(
    Light light,
    LightPoints lightPoints,
    GBufferStandard gBuffer, 
    LTCTerms ltcTerms,
    float3 viewDirection,
    float3 surfacePosition)
{
    float specularLobe = LTCEvaluateRectangle(gBuffer.Normal, viewDirection, surfacePosition, ltcTerms.MInvSpecular, lightPoints.Points);
    float diffuseLobe = LTCEvaluateRectangle(gBuffer.Normal, viewDirection, surfacePosition, ltcTerms.MInvDiffuse, lightPoints.Points);

    return ApplyMaterialAndLightingToLTCLobes(specularLobe, diffuseLobe, light, ltcTerms);
}

LightPoints ComputeLightPoints(Light light, float3 surfacePositionWS)
{
    LightPoints points;

    float halfWidth = light.Width * 0.5;
    float halfHeight = light.Height * 0.5;

    // Add a small value so that width and height is never truly equal.
    // LTC disk evaluation code has nasty numerical errors (NaNs) in a corner case 
    // when width and height are fully equal and disk is rotated
    // strictly toward the surface point.
    // A slightly more wide light is almost imperceptible, so I guess it will have to do.
    //
    halfWidth += light.LightType == LightTypeSphere ? 0.5 : 0.0;

    // Get billboard points at the origin
    float3 p0 = float3(halfWidth, -halfHeight, 0.0);
    float3 p1 = float3(halfWidth, halfHeight, 0.0);
    float3 p2 = float3(-halfWidth, halfHeight, 0.0);
    float3 p3 = float3(-halfWidth, -halfHeight, 0.0);

    float3 lightOrientation = light.LightType == LightTypeSphere ?
        normalize(surfacePositionWS.xyz - light.Position.xyz) : // Spherical light is a disk oriented towards the surface
        light.Orientation.xyz;

    float3x3 lightRotation = RotationMatrix3x3(lightOrientation);
    points.LightRotation = lightRotation;

    // Rotate around origin
    p0 = mul(lightRotation, p0);
    p1 = mul(lightRotation, p1);
    p2 = mul(lightRotation, p2);
    p3 = mul(lightRotation, p3);

    // Move points to light's location
    // Clockwise to match LTC convention
    points.Points[0] = p3.xyz + light.Position.xyz;
    points.Points[1] = p2.xyz + light.Position.xyz;
    points.Points[2] = p1.xyz + light.Position.xyz;
    points.Points[3] = p0.xyz + light.Position.xyz;

    return points;
}

//-------------------------------------------------------------------------------//
// Rectangular Lights
//-------------------------------------------------------------------------------//

// Sampling the Solid Angle of Area Light Sources
// https://schuttejoe.github.io/post/arealightsampling/

// Solid-angle rectangular light sampling (as opposed to area sampling).
// Yields lower variance and faster convergence.
// An Area-Preserving Parametrization for Spherical Rectangles:
// https://www.arnoldrenderer.com/research/egsr2013_spherical_rectangle.pdf
//
RectLightSolidAngleSamplingInputs ComputeRectLightSolidAngleSamplingInputs(LightPoints lightPoints, float3 surfacePositionWS)
{
    RectLightSolidAngleSamplingInputs samplingInputs;

    float3 ex = lightPoints.Points[1] - lightPoints.Points[0];
    float3 ey = lightPoints.Points[3] - lightPoints.Points[0];
    float exl = length(ex);
    float eyl = length(ey);

    // Compute local reference system 'R'
    samplingInputs.x = ex / exl;
    samplingInputs.y = ey / eyl;
    samplingInputs.z = cross(samplingInputs.x, samplingInputs.y);

    // Compute rectangle coords in local reference system (local to surface position)
    float3 p0LocalToSurface = lightPoints.Points[0] - surfacePositionWS;
    samplingInputs.z0 = dot(p0LocalToSurface, samplingInputs.z);

    // Flip 'z' to make it point against 'Q'
    if (samplingInputs.z0 > 0.0f)
    {
        samplingInputs.z = -samplingInputs.z;
        samplingInputs.z0 = -samplingInputs.z0;
    }

    samplingInputs.z0sq = samplingInputs.z0 * samplingInputs.z0;
    samplingInputs.x0 = dot(p0LocalToSurface, samplingInputs.x);
    samplingInputs.y0 = dot(p0LocalToSurface, samplingInputs.y);
    samplingInputs.x1 = samplingInputs.x0 + exl;
    samplingInputs.y1 = samplingInputs.y0 + eyl;
    samplingInputs.y0sq = samplingInputs.y0 * samplingInputs.y0;
    samplingInputs.y1sq = samplingInputs.y1 * samplingInputs.y1;

    // create vectors to four vertices
    float3 v00 = float3(samplingInputs.x0, samplingInputs.y0, samplingInputs.z0);
    float3 v01 = float3(samplingInputs.x0, samplingInputs.y1, samplingInputs.z0);
    float3 v10 = float3(samplingInputs.x1, samplingInputs.y0, samplingInputs.z0);
    float3 v11 = float3(samplingInputs.x1, samplingInputs.y1, samplingInputs.z0);

    // compute normals to edges
    float3 n0 = normalize(cross(v00, v10));
    float3 n1 = normalize(cross(v10, v11));
    float3 n2 = normalize(cross(v11, v01));
    float3 n3 = normalize(cross(v01, v00));

    // compute internal angles (gamma_i)
    float g0 = acos(-dot(n0, n1));
    float g1 = acos(-dot(n1, n2));
    float g2 = acos(-dot(n2, n3));
    float g3 = acos(-dot(n3, n0));

    // Compute predefined constants
    samplingInputs.b0 = n0.z;
    samplingInputs.b1 = n2.z;
    samplingInputs.b0sq = samplingInputs.b0 * samplingInputs.b0;
    samplingInputs.k = 2.0f * Pi - g2 - g3;

    // Compute solid angle from internal angles
    samplingInputs.SolidAngle = g0 + g1 - samplingInputs.k;
    samplingInputs.SurfacePoint = surfacePositionWS;

    return samplingInputs;
}

float3 RectangularLightSampleVector(RectLightSolidAngleSamplingInputs samplingInputs, float u, float v)
{
    // 1. compute 'cu'
    float au = u * samplingInputs.SolidAngle + samplingInputs.k;
    float fu = (cos(au) * samplingInputs.b0 - samplingInputs.b1) / sin(au);
    float cu = 1.0f / sqrt(fu * fu + samplingInputs.b0sq) * (fu > 0.0f ? 1.0f : -1.0f);
    cu = clamp(cu, -1.0f, 1.0f); // avoid NaNs

    // 2. compute 'xu'
    float xu = -(cu * samplingInputs.z0) / sqrt(1.0f - cu * cu);
    xu = clamp(xu, samplingInputs.x0, samplingInputs.x1); // avoid Infs

    // 3. compute 'yv'
    float d = sqrt(xu * xu + samplingInputs.z0sq);
    float h0 = samplingInputs.y0 / sqrt(d * d + samplingInputs.y0sq);
    float h1 = samplingInputs.y1 / sqrt(d * d + samplingInputs.y1sq);
    float hv = h0 + v * (h1 - h0), hv2 = hv * hv;
    float yv = (hv2 < 1.0f - 1e-6f) ? (hv * d) / sqrt(1.0f - hv2) : samplingInputs.y1;

    // 4. transform (xu, yv, z0) to world coords
    float3 pointOnLight = samplingInputs.SurfacePoint + xu * samplingInputs.x + yv * samplingInputs.y + samplingInputs.z0 * samplingInputs.z;
    float3 toLightVector = normalize(pointOnLight - samplingInputs.SurfacePoint);

    return toLightVector;
}

RectLightAreaSamplingInputs ComputeRectLightAreaSamplingInputs(Light light, float3 surfacePoint, float3x3 lightRotation)
{
    RectLightAreaSamplingInputs inputs;
    inputs.LightPointLocalToWorldRotation = lightRotation;
    inputs.Area = light.Width * light.Height;
    inputs.SurfacePoint = surfacePoint;
    return inputs;
}

float3 RectangularLightRandomPoint(Light light, RectLightAreaSamplingInputs samplingInputs, float u, float v)
{
    float x = -light.Width * 0.5 + u * light.Width;
    float y = -light.Height * 0.5 + v * light.Height;

    float3 randomPoint = float3(x, y, 0.0);
    randomPoint = mul(samplingInputs.LightPointLocalToWorldRotation, randomPoint);
    randomPoint += light.Position.xyz;

    return randomPoint;
}

LightSample SampleRectangularLight(Light light, LightPoints lightPoints, float samplePDF, float3 surfacePoint, float3 samplingVector)
{
    Ray ray = InitRay(surfacePoint, samplingVector);
    Plane plane = InitPlane(light.Orientation.xyz, light.Position.xyz); 
    LightSample lightSample = ZeroLightSample();

    float3 intersectionPoint;
    if (RayPlaneIntersection(plane, ray, intersectionPoint))
    {
        float3 localPoint = intersectionPoint - light.Position.xyz;
        float3 zAlignedPoint = mul(transpose(lightPoints.LightRotation), localPoint);

        float hw = light.Width * 0.5;
        float hh = light.Height * 0.5;

        bool pointInsideRect =
            all(zAlignedPoint.xy >= float2(-hw, -hh)) &&
            all(zAlignedPoint.xy <= float2(hw, hh));

        if (pointInsideRect)
        {
            lightSample.IntersectionPoint = intersectionPoint;
            lightSample.PDF = samplePDF;
        }
    }

    return lightSample;
}

LightSample SampleRectangularLight(Light light, RectLightSolidAngleSamplingInputs samplingInputs, LightPoints lightPoints, float3 samplingVector)
{
    return SampleRectangularLight(light, lightPoints, 1.0 / samplingInputs.SolidAngle, samplingInputs.SurfacePoint, samplingVector);
}

LightSample SampleRectangularLight(Light light, RectLightAreaSamplingInputs samplingInputs, LightPoints lightPoints, float3 samplingVector)
{
    return SampleRectangularLight(light, lightPoints, 1.0 / samplingInputs.Area, samplingInputs.SurfacePoint, samplingVector);
}

//-------------------------------------------------------------------------------//
// Spherical Lights
//-------------------------------------------------------------------------------//

// Solid-angle sampling of spherical lights
// https://schuttejoe.github.io/post/arealightsampling/
//
ShereLightSolidAngleSamplingInputs ComputeSphericalLightSamplingInputs(Light light, float3 surfacePositionWS)
{
    ShereLightSolidAngleSamplingInputs samplingInputs;
    samplingInputs.SurfacePoint = surfacePositionWS;

    float3 w = light.Position.xyz - samplingInputs.SurfacePoint;
    float distanceToCenter = length(w);
    w /= distanceToCenter;

    float radius = light.Height * 0.5;
    float radiusOverDist = radius / distanceToCenter;

    samplingInputs.Q = sqrt(1.0f - radiusOverDist * radiusOverDist);
    samplingInputs.SolidAngle = TwoPi * (1.0f - samplingInputs.Q);
    samplingInputs.SampleLocalToWorldRotation = RotationMatrix3x3(w);

    return samplingInputs;
}

float3 SphericalLightSampleVector(ShereLightSolidAngleSamplingInputs samplingInputs, float u1, float u2)
{
    float theta = acos(1.0 - u1 + u1 * samplingInputs.Q);
    float phi = TwoPi * u2;

    float3 localSampleVector = SphericalToCartesian_ZUp(theta, phi);
    float3 worldSampleVector = mul(samplingInputs.SampleLocalToWorldRotation, localSampleVector);

    return worldSampleVector;
}

LightSample SampleSphericalLight(Light light, ShereLightSolidAngleSamplingInputs samplingInputs, float3 samplingVector)
{
    Ray ray = InitRay(samplingInputs.SurfacePoint, samplingVector);
    Sphere sphere = InitSphere(light.Position.xyz, light.Width * 0.5);
    LightSample lightSample = ZeroLightSample();

    float3 xp;
    if (RaySphereIntersection(sphere, ray, xp))
    {
        lightSample.IntersectionPoint = xp;
        lightSample.PDF = 1.0f / samplingInputs.SolidAngle;
    }

    return lightSample;
}

//-------------------------------------------------------------------------------//
// Disk Lights
//-------------------------------------------------------------------------------//

// Solid-angle sampling of spherical ellipsoids is described in the research below,
// but, IMO, it's too complicated for a real-time case, so treat elliptical light as a rect light
//
// Area-Preserving Parameterizations for Spherical Ellipses
// http://giga.cps.unizar.es/~iguillen/projects/EGSR2017_Spherical_Ellipses/
// https://www.arnoldrenderer.com/research/egsr2017_spherical_ellipse.pdf

// Disk light is sampled as rect light with corresponding PDF of a rect light, but intersection test still should be made with an ellipse
LightSample SampleEllipticalLight(Light light, RectLightSolidAngleSamplingInputs samplingInputs, LightPoints lightPoints, float3 samplingVector)
{
    Ray ray = InitRay(samplingInputs.SurfacePoint, samplingVector);
    Plane plane = InitPlane(light.Orientation.xyz, light.Position.xyz);
    LightSample lightSample = ZeroLightSample();

    float3 intersectionPoint;
    if (RayPlaneIntersection(plane, ray, intersectionPoint))
    {
        float3 localPoint = intersectionPoint - light.Position.xyz;
        float3 zAlignedPoint = mul(transpose(lightPoints.LightRotation), localPoint);

        if (IsPointInsideEllipse(zAlignedPoint.xy, 0.xx, float2(light.Width, light.Height)))
        {
            lightSample.IntersectionPoint = intersectionPoint;
            lightSample.PDF = 1.0 / samplingInputs.SolidAngle;
        }
    }

    return lightSample;
}

#endif