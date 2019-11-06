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

float3 DecodeGBufferCookTorranceNormal(GBufferEncoded encodedGBuffer)
{
    uint3 encoded = encodedGBuffer.MaterialData.xyz;
    uint metalnessAONormalZ = encoded.y;
    float normalZ = UnpackSnorm2x16(metalnessAONormalZ, 1.0).y;
    float2 normalXY = UnpackSnorm2x16(encoded.z, 1.0);
    return float3(normalXY, normalZ);
}
