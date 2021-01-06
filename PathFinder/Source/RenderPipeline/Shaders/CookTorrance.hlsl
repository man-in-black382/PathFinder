#ifndef _BRDF__
#define _BRDF__

#include "Constants.hlsl"

// The Fresnel equation describes the ratio of surface reflection at different surface angles.
// (This is an approximation of Fresnels' equation, called Fresnel-Schlick)
// Describes the ratio of light that gets reflected over the light that gets refracted,
// which varies over the angle we're looking at a surface.
//
// f0 - reflection coefficient for light incoming parallel to the normal
// (i.e., the value of the Fresnel term when  = 0 or minimal reflection)
//
// f90 - reflection coefficient for light incoming perpendicular to the normal
// (should be 1 in realistic scenario, but left as a variable rather then a constant to add flexibility)
//
// https://en.wikipedia.org/wiki/Schlick%27s_approximation
//
float3 FresnelSchlick(float3 f0, float3 f90, float cosPhi)
{
    return f0 + (f90 - f0) * pow(1.0 - cosPhi, 5.0);
}

// Trowbridge-Reitz GGX normal distribution function
// Statistically approximates the ratio of microfacets aligned to some (halfway) vector h
//
float NormalDistributionTrowbridgeReitzGGX(float NdotH, float roughness)
{
    float a2 = roughness * roughness;
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = Pi * denom * denom;

    return nom / denom;
}

// Statistically approximates the ratio of microfacets that overshadow each other causing light rays to lose their energy in the process.
// Combination of the GGX and Schlick-Beckmann approximation known as Schlick-GGX.
//
float GeometrySchlickGGX(float NdotV, float roughness) 
{
    float a = (roughness + 1.0);

    // Here k is a remapping of roughness based on whether we're using the geometry function for either direct lighting or IBL lighting
    float Kdirect = (a * a) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - Kdirect) + Kdirect;

    return nom / denom;
}

// To effectively approximate the geometry we need to take account of both the view direction (geometry obstruction) and the light direction vector (geometry shadowing).
// We can take both into account using Smith's method:
float GeometrySmithGGX(float NdotL, float NdotV, float roughness)
{
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);

    return ggx1 * ggx2;
}

// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// 3.1.5 Frostbite standard model
//
float GeometrySmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
    // Original formulation of G_SmithGGX Correlated 
    // lambda_v = (-1 + sqrt(alphaG2 * (1 - NdotL2) / NdotL2 + 1)) * 0.5f; 
    // lambda_l = (-1 + sqrt(alphaG2 * (1 - NdotV2) / NdotV2 + 1)) * 0.5f; 
    // G_SmithGGXCorrelated = 1 / (1 + lambda_v + lambda_l); 
    // V_SmithGGXCorrelated = G_SmithGGXCorrelated / (4.0f * NdotL * NdotV); 

    // This is the optimized version 
    float alphaG2 = alphaG * alphaG;
    // Caution: the "NdotL *" and "NdotV *" are explicitely inversed , this is not a mistake. 
    float Lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
    float Lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

    return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

// Renormalized Disney diffuse formula to be energy conserving 
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// 3.1.3 Energy conservation
//
float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float energyBias = lerp(0, 0.5, linearRoughness);
    float energyFactor = lerp(1.0, 1.0 / 1.51, linearRoughness);
    float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
    float3 f0 = float3(1.0f, 1.0f, 1.0f);
    float lightScatter = FresnelSchlick(f0, fd90, NdotL).r;
    float viewScatter = FresnelSchlick(f0, fd90, NdotV).r;

    return lightScatter * viewScatter * energyFactor / Pi;
}

float3 BRDF(float3 N, float3 V, float3 H, float3 L, float roughness, float3 albedo, float metalness, float3 radiance)
{
    // Based on observations by Disney and adopted by Epic Games
    // the lighting looks more correct squaring the roughness
    // in both the geometry and normal distribution function.
    float roughness2 = roughness * roughness;

    float NdotL = saturate(dot(N, L)); 
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));

    float NDF = NormalDistributionTrowbridgeReitzGGX(NdotH, roughness2);
    float G = GeometrySmithGGXCorrelated(NdotL, NdotV, roughness2);

    const float BaseDielectricReflectivity = 0.04;

    float3 f0 = lerp(BaseDielectricReflectivity.xxx, albedo, metalness);
    float3 F = FresnelSchlick(f0, 1.0, NdotH);

    float3 specular = (NDF * G * F) / (4.0 * NdotL * NdotV + 0.001); // Add 0.001 to prevent division by 0
    float3 diffuse = DisneyDiffuse(NdotV, NdotL, NdotH, roughness) * (1.0 - metalness) * albedo; 

    float3 cosineWeightedRadiance = radiance * NdotL;

    return (diffuse + specular) * cosineWeightedRadiance;
}

#endif