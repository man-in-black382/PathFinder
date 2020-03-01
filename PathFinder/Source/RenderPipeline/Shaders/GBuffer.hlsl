#ifndef _GBuffer__
#define _GBuffer__

#include "Packing.hlsl"

static const uint GBufferTypeStandard = 0;
static const uint GBufferTypeEmissive = 1;

struct GBufferEncoded
{
    uint4 MaterialData;
};

struct GBufferPixelOut
{
    uint4 MaterialData : SV_Target0;
};

struct GBufferStandard
{
    float3 Albedo;
    float3 Normal;
    float Roughness;
    float Metalness;
    float AO;
    uint MaterialIndex;
};

struct GBufferEmissive
{
    float3 LuminousIntensity;
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
// |        Normal X      |        Normal Y      |     Material Index   |     GBuffer type     |
// |                      |                      |                      |                      |
// |        2 Bytes       |        2 Bytes       |        2 Bytes       |        2 Bytes       |
// |______________________|______________________|_____________________________________________|
// |________Third component of output UVEC4______|_______Fourth component of output UVEC4______|

GBufferEncoded EncodeGBuffer(GBufferStandard gBuffer)
{
    uint albedoRoughness = Encode8888(float4(gBuffer.Albedo, gBuffer.Roughness));
    uint metalnessAO = Encode8888(float4(gBuffer.Metalness, gBuffer.AO, 0.0, 0.0)); // Metalness and AO are packed to MSB

    uint normalZ = PackSnorm2x16(0.0, gBuffer.Normal.z, 1.0); // Pack Z to LSB
    uint normalXY = PackSnorm2x16(gBuffer.Normal.x, gBuffer.Normal.y, 1.0);

    uint metalnessAONormalZ = metalnessAO | normalZ;

    uint materialIndexAndGBufferType = 0;
    materialIndexAndGBufferType |= (gBuffer.MaterialIndex << 16); // Move material index to 16 MSB
    materialIndexAndGBufferType |= GBufferTypeStandard; // Store GBuffer type in 16 LSB

    GBufferEncoded encoded;
    encoded.MaterialData = uint4(albedoRoughness, metalnessAONormalZ, normalXY, materialIndexAndGBufferType);
    return encoded;
}

GBufferEncoded EncodeGBuffer(GBufferEmissive gBuffer)
{
    GBufferEncoded encoded;
    encoded.MaterialData = uint4(asuint(gBuffer.LuminousIntensity.x), asuint(gBuffer.LuminousIntensity.y), asuint(gBuffer.LuminousIntensity.z), GBufferTypeEmissive);
    return encoded;
}

uint DecodeGBufferType(GBufferEncoded encoded)
{
    return encoded.MaterialData.w & 0x0000FFFF;
}

GBufferStandard DecodeGBufferStandard(GBufferEncoded encodedGBuffer)
{
     GBufferStandard gBuffer;  

     uint4 encoded = encodedGBuffer.MaterialData;      
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

     gBuffer.MaterialIndex = encoded.w >> 16;

     return gBuffer;
 }

float3 DecodeGBufferStandardNormal(GBufferEncoded encodedGBuffer)
{
    uint3 encoded = encodedGBuffer.MaterialData.xyz;
    uint metalnessAONormalZ = encoded.y;
    float normalZ = UnpackSnorm2x16(metalnessAONormalZ, 1.0).y;
    float2 normalXY = UnpackSnorm2x16(encoded.z, 1.0);
    return float3(normalXY, normalZ);
}

GBufferEmissive DecodeGBufferEmissive(GBufferEncoded encodedGBuffer)
{
    GBufferEmissive gBuffer;
    gBuffer.LuminousIntensity.x = asfloat(encodedGBuffer.MaterialData.x);
    gBuffer.LuminousIntensity.y = asfloat(encodedGBuffer.MaterialData.y);
    gBuffer.LuminousIntensity.z = asfloat(encodedGBuffer.MaterialData.z);
    return gBuffer;
}

#endif