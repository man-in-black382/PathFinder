struct PassData
{
    uint InstanceTableIndex;
};

#define PassDataType PassData

#include "InstanceData.hlsl"
#include "BaseRootSignature.hlsl"
#include "GBuffer.hlsl"
#include "ColorConversion.hlsl"

StructuredBuffer<InstanceData> InstanceTable : register(t0);

//------------------------  Vertex  ------------------------------//

struct VertexIn
{
    float4 Position : POSITION0;
    float3 Normal : NORMAL0;
    float2 UV : TEXCOORD0;
    float3 Tangent : TANGENT0;
    float3 Bitangent : BITANGENT0;
};

struct VertexOut
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;  
    float3x3 TBN : TBN_MATRIX;
};

float3x3 BuildTBNMatrix(VertexIn vertex, InstanceData instanceData)
{
    float4x4 normalMatrix = instanceData.NormalMatrix;

    float3 T = normalize(mul(normalMatrix, float4(vertex.Tangent, 0.0))).xyz;
    float3 B = normalize(mul(normalMatrix, float4(vertex.Bitangent, 0.0))).xyz;
    float3 N = normalize(mul(normalMatrix, float4(vertex.Normal, 0.0))).xyz;

    return float3x3(T, B, N);
}

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    InstanceData instanceData = InstanceTable[PassDataCB.InstanceTableIndex];
    float4x4 MVP = mul(FrameDataCB.CameraViewProjection, instanceData.ModelMatrix);

    vout.Position = mul(MVP, vin.Position);
    vout.UV = vin.UV;
    vout.TBN = BuildTBNMatrix(vin, instanceData);

    return vout;
}

//------------------------  Pixel  ------------------------------//

struct PixelOut
{
    uint4 MaterialData : SV_Target0;
};

float3 FetchAlbedoMap(VertexOut vertex, InstanceData instanceData)
{
    Texture2D albedoMap = Textures2D[instanceData.AlbedoMapIndex];
    return LinearFromSRGB(albedoMap.Sample(AnisotropicClampSampler, vertex.UV).rgb);
}

float3 FetchNormalMap(VertexOut vertex, InstanceData instanceData)
{
    Texture2D normalMap = Textures2D[instanceData.NormalMapIndex];
    float3 normal = normalMap.Sample(AnisotropicClampSampler, vertex.UV).xyz;
    return normalize(mul(vertex.TBN, (normal * 2.0 - 1.0)));
}

float FetchMetallnessMap(VertexOut vertex, InstanceData instanceData)
{
    Texture2D metalnessMap = Textures2D[instanceData.MetalnessMapIndex];
    return metalnessMap.Sample(AnisotropicClampSampler, vertex.UV).r;
}

float FetchRoughnessMap(VertexOut vertex, InstanceData instanceData)
{
    Texture2D roughnessMap = Textures2D[instanceData.RoughnessMapIndex];
    return roughnessMap.Sample(AnisotropicClampSampler, vertex.UV).r;
}

float FetchAOMap(VertexOut vertex, InstanceData instanceData)
{
    Texture2D aoMap = Textures2D[instanceData.AOMapIndex];
    return aoMap.Sample(AnisotropicClampSampler, vertex.UV).r;
}

PixelOut PSMain(VertexOut pin)
{
    InstanceData instanceData = InstanceTable[PassDataCB.InstanceTableIndex];

    GBufferCookTorrance gBufferData;
    gBufferData.Albedo = FetchAlbedoMap(pin, instanceData);
    gBufferData.Normal = FetchNormalMap(pin, instanceData);
    gBufferData.Metalness = FetchMetallnessMap(pin, instanceData);
    gBufferData.Roughness = FetchRoughnessMap(pin, instanceData);
    gBufferData.AO = FetchAOMap(pin, instanceData);

    GBufferEncoded encoded = EncodeCookTorranceMaterial(gBufferData);

    PixelOut pixelOut;
    pixelOut.MaterialData = encoded.MaterialData;

    return pixelOut;
}