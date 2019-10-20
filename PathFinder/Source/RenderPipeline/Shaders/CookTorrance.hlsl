static const float PI = 3.1415926535897932384626433832795;

// The Fresnel equation describes the ratio of surface reflection at different surface angles.
// (This is an approximation of Fresnels' equation, called Fresnel-Schlick)
// Describes the ratio of light that gets reflected over the light that gets refracted,
// which varies over the angle we're looking at a surface.
//
float3 FresnelSchlick(float3 V, float3 H, float3 albedo, float metallic)
{
    // F0 - base reflectivity of a surface, a.k.a. surface reflection at zero incidence
    // or how much the surface reflects if looking directly at it
    float3 F0 = float3(0.04, 0.04, 0.04); // 0.04 is a commonly used constant for dielectrics
    F0 = lerp(F0, albedo, metallic);

    float cosTheta = max(dot(H, V), 0.0);

    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Trowbridge-Reitz GGX normal distribution function
// Statistically approximates the ratio of microfacets aligned to some (halfway) vector h
//
float NormalDistributionGGX(float3 N, float3 H, float roughness)
{
    float a2 = roughness * roughness;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

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
float GeometrySmith(float NdotL, float NdotV, float roughness)
{
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);

    return ggx1 * ggx2;
}

float3 CookTorranceBRDF(float3 N, float3 V, float3 H, float3 L, float roughness, float3 albedo, float metallic, float3 radiance)
{
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float NDF = NormalDistributionGGX(N, H, roughness);
    float G = GeometrySmith(NdotL, NdotV, roughness);
    float3 F = FresnelSchlick(V, H, albedo, metallic);

    float3 nom = NDF * G * F;
    float denom = 4.0 * NdotL * NdotV + 0.001; // Add 0.001 to prevent division by 0
    float3 specular = nom / denom;

    float3 Ks = F;                // Specular (reflected) portion
    float3 Kd = float3(1.0, 1.0, 1.0) - Ks; // Diffuse (refracted) portion

    Kd *= 1.0 - metallic;  // This will turn diffuse component of metallic surfaces to 0

    float3 cosineRadiance = radiance * NdotL;

    // Lambertian diffuse component with indirect light applied
    float3 diffuse = Kd * (albedo / PI) * cosineRadiance;

    specular *= cosineRadiance;

    return diffuse + specular;
}
