#ifndef _ImportanceSampling__
#define _ImportanceSampling__

#include "Constants.hlsl"
#include "BRDF.hlsl"

// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// Page 69
float3 GGXDominantDirection(float3 N, float3 V, float roughness)
{
    float smoothness = 1.0 - roughness;
    float f = smoothness * (sqrt(smoothness) + roughness);
    float3 R = reflect(-V, N);
    float3 direction = lerp(N, R, f);

    return direction;
}

float GGXLobeHalfAngle(float roughness)
{
    float roughness2 = roughness * roughness;
    float angle = (90.0 * roughness2) / (1.0 + roughness2);
    return angle * PiOver180;
}

// Following Wo, Wi, Wm are vectors in tangent space of the surface 

// https://hal.archives-ouvertes.fr/hal-01509746/document
float3 SampleGgxVndfAnisotropic(float3 wo, float ax, float ay, float u1, float u2)
{
    // -- Stretch the view vector so we are sampling as though roughness==1
    float3 v = normalize(float3(wo.x * ax, wo.y * ay, wo.z));

    // -- Build an orthonormal basis with v, t1, and t2
    float3 t1 = (v.z < 0.9999f) ? normalize(cross(v, ZAxis)) : XAxis;
    float3 t2 = cross(t1, v);

    // -- Choose a point on a disk with each half of the disk weighted proportionally to its projection onto direction v
    float a = 1.0f / (1.0f + v.y);
    float r = sqrt(u1);
    float phi = (u2 < a) ? (u2 / a) * Pi : Pi + (u2 - a) / (1.0f - a) * Pi;
    float p1 = r * cos(phi);
    float p2 = r * sin(phi) * ((u2 < a) ? 1.0f : v.y);

    // -- Calculate the normal in this stretched tangent space
    float3 n = p1 * t1 + p2 * t2 + sqrt(max(0.0f, 1.0f - p1 * p1 - p2 * p2)) * v;

    // -- unstretch and normalize the normal
    return normalize(float3(ax * n.x, n.y, ay * n.z));
}

float3 SampleGgxVndf(float3 wo, float roughness, float u1, float u2)
{
    return SampleGgxVndfAnisotropic(wo, roughness, roughness, u1, u2);
}

float GgxVndfPdf(float3 wo, float3 wm, float3 wi, float a)
{
    float absDotNL = AbsCosTheta(wi);
    float absDotLH = abs(dot(wm, wi));

    float G1 = SeparableSmithGGXG1(wo, a);
    float D = NormalDistributionTrowbridgeReitzGGXIsotropic(wm, a);

    return G1 * absDotLH * D / absDotNL;
}

float GgxVndfAnisotropicPdf(float3 wi, float3 wm, float3 wo, float ax, float ay)
{
    float absDotNL = AbsCosTheta(wi);
    float absDotLH = abs(dot(wm, wi));

    float G1 = SeparableSmithGGXG1(wo, wm, ax, ay);
    float D = NormalDistributionTrowbridgeReitzGGXAnisotropic(wm, ax, ay);

    return G1 * absDotLH * D / absDotNL;
}

void GgxVndfAnisotropicPdf(float3 wi, float3 wm, float3 wo, float ax, float ay,
    inout float forwardPdfW, inout float reversePdfW)
{
    float D = NormalDistributionTrowbridgeReitzGGXAnisotropic(wm, ax, ay);

    float absDotNL = AbsCosTheta(wi);
    float absDotHL = abs(dot(wm, wi));
    float G1v = SeparableSmithGGXG1(wo, wm, ax, ay);
    forwardPdfW = G1v * absDotHL * D / absDotNL;

    float absDotNV = AbsCosTheta(wo);
    float absDotHV = abs(dot(wm, wo));
    float G1l = SeparableSmithGGXG1(wi, wm, ax, ay);
    reversePdfW = G1l * absDotHV * D / absDotNV;
}

#endif