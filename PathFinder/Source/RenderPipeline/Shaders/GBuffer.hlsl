#include "Packing.hlsl"

struct GBufferEncoded
{
    uint4 MaterialData;
};

struct GBufferCookTorrance
{
    float3 Albedo;
    float3 Normal;
    float Roughness;
    float Metalness;
    float AO;
};

// Packing scheme
//
// | Albedo R | Albedo G | Albedo B | Roughness | Metalness |    AO    |        Normal Z      |
// |          |          |          |           |           |          |                      |
// |  1 Byte  |  1 Byte  |  1 Byte  |  1 Byte   |  1 Byte   |  1 Byte  |        2 Bytes       |
// |__________|__________|__________|___________|___________|__________|______________________|
// |______First component of output UVEC4_______|_______Second component of output UVEC4______|
//
//
// |        Normal X      |        Normal Y      |                  Material type              |
// |                      |                      |                                             |
// |        2 Bytes       |        2 Bytes       |                    4 bytes                  |
// |______________________|______________________|_____________________________________________|
// |________Third component of output UVEC4______|_______Fourth component of output UVEC4______|

GBufferEncoded EncodeCookTorranceMaterial(GBufferCookTorrance gBuffer)
{
    uint albedoRoughness = Encode8888(float4(gBuffer.Albedo, gBuffer.Roughness));
    uint metalnessAO = Encode8888(float4(gBuffer.Metalness, gBuffer.AO, 0.0, 0.0)); // Metalness and AO are packed to MSB

    uint normalZ = PackSnorm2x16(0.0, gBuffer.Normal.z, 1.0); // Pack Z to LSB
    uint normalXY = PackSnorm2x16(gBuffer.Normal.x, gBuffer.Normal.y, 1.0);

    uint metalnessAONormalZ = metalnessAO | normalZ;

    GBufferEncoded encoded;
    encoded.MaterialData = uint4(albedoRoughness, metalnessAONormalZ, normalXY, 0);
    return encoded;
}

// GBufferCookTorrance packing scheme
//
// | Albedo R | Albedo G | Albedo B | Roughness | Metalness |    AO    |        Normal Z      |
// |          |          |          |           |           |          |                      |
// |  1 Byte  |  1 Byte  |  1 Byte  |   1 Byte  |  1 Byte   |  1 Byte  |        2 Bytes       |
// |__________|__________|__________|___________|___________|__________|______________________|
// |______First component of output UVEC3_______|_______Second component of output UVEC3______|
//
//
// |        Normal X      |        Normal Y      |
// |                      |                      |
// |        2 Bytes       |        2 Bytes       |
// |______________________|______________________|
// |________Third component of output UVEC3______|
//

GBufferCookTorrance DecodeGBufferCookTorrance(GBufferEncoded encodedGBuffer)
{
     GBufferCookTorrance gBuffer;

     uint3 encoded = encodedGBuffer.MaterialData.xyz;
     float4 albedoRoughness = Decode8888(encoded.x);
     uint metalnessAONormalZ = encoded.y;
     float2 metalnessAO = Decode8888(metalnessAONormalZ).xy;
     float normalZ = UnpackSnorm2x16(metalnessAONormalZ, 1.0).y;
     float2 normalXY = UnpackSnorm2x16(encoded.z, 1.0);

     gBuffer.Albedo    = albedoRoughness.rgb;
     gBuffer.Normal    = float3(normalXY, normalZ);
     gBuffer.Roughness = albedoRoughness.a;
     gBuffer.Metalness = metalnessAO.r;
     gBuffer.AO        = metalnessAO.g;

     return gBuffer;
 }

//float3 ReconstructWorldPosition(float hyperbolicDepth, // Depth value after viewport transformation in [0; 1] range
//                              vloat2 normTexCoords, // Normalized texture coordinates [0; 1]
//                              mat4 inverseView, // Inverse camera view matrix
//                              mat4 inverseProjection) // Inverse camera projection matrix
//{
//    // Depth range in NDC is [-1; 1]
//    // Default value for glDepthRange is [-1; 1]
//    // OpenGL uses values from glDepthRange to transform depth to [0; 1] range during viewport transformation
//    // To reconstruct world position using inverse camera matrices we need depth in [-1; 1] range again
//    float z = hyperbolicDepth * 2.0 - 1.0;
//    vec2 xy = normTexCoords * 2.0 - 1.0;
//
//    vec4 clipSpacePosition = vec4(xy, z, 1.0);
//    vec4 viewSpacePosition = inverseProjection * clipSpacePosition;
//
//    // Perspective division
//    viewSpacePosition /= viewSpacePosition.w;
//
//    vec4 worldSpacePosition = inverseView * viewSpacePosition;
//
//    return worldSpacePosition.xyz;
//}

//float3 ReconstructWorldPosition(sampler2D depthSampler, vec2 normTexCoords, mat4 inverseView, mat4 inverseProjection)
//{
//    float depth = texture(depthSampler, normTexCoords).r;
//    return ReconstructWorldPosition(depth, normTexCoords, inverseView, inverseProjection);
//}
//
//float3 DecodeGBufferCookTorranceNormal(uvec4 materialData)
//{
//    uint3 encoded = materialData.xyz;
//    uint metalnessAONormalZ = encoded.y;
//    float normalZ = UnpackSnorm2x16(metalnessAONormalZ, 1.0).y;
//    float2 normalXY = UnpackSnorm2x16(encoded.z, 1.0);
//    return float3(normalXY, normalZ);
//}
