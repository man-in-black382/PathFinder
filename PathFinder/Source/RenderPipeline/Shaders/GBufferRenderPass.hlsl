struct PassData
{
    uint InstanceTableIndex;
};

#define PassDataType PassData

#include "InstanceData.hlsl"
#include "BaseRootSignature.hlsl"
#include "GBuffer.hlsl"
#include "ColorConversion.hlsl"
#include "Vertices.hlsl"

StructuredBuffer<Vertex1P1N1UV1T1BT> UnifiedVertexBuffer : register(t0);
StructuredBuffer<IndexU32> UnifiedIndexBuffer : register(t1);
StructuredBuffer<InstanceData> InstanceTable : register(t2);

//------------------------  Vertex  ------------------------------//

struct VertexOut
{
    float4 Position : SV_POSITION;
    float3 ViewDirectionTS : VIEW_VECTOR_TS;
    float2 UV : TEXCOORD0;  
    float3x3 TBN : TBN_MATRIX;

    float3 Normal : NORMAL;
};

float3x3 BuildTBNMatrix(Vertex1P1N1UV1T1BT vertex, InstanceData instanceData)
{
    float4x4 normalMatrix = instanceData.NormalMatrix;

    float3 T = normalize(mul(normalMatrix, float4(normalize(vertex.Tangent), 0.0))).xyz;
    float3 B = normalize(mul(normalMatrix, float4(normalize(vertex.Bitangent), 0.0))).xyz;
    float3 N = normalize(mul(normalMatrix, float4(normalize(vertex.Normal), 0.0))).xyz;

    return float3x3(T, B, N);
}

VertexOut VSMain(uint indexId : SV_VertexID)
{
    VertexOut vout;
    
    InstanceData instanceData = InstanceTable[PassDataCB.InstanceTableIndex];

    // Load index and vertex
    IndexU32 index = UnifiedIndexBuffer[instanceData.UnifiedIndexBufferOffset + indexId];
    Vertex1P1N1UV1T1BT vertex = UnifiedVertexBuffer[instanceData.UnifiedVertexBufferOffset + index.Index];

    float3x3 TBN = BuildTBNMatrix(vertex, instanceData);
    float3x3 TBNInverse = transpose(TBN);

    float4 WSPosition = mul(instanceData.ModelMatrix, vertex.Position);
    float3 viewVector = normalize(FrameDataCB.CameraPosition.xyz - WSPosition.xyz);

    vout.Position = mul(FrameDataCB.CameraViewProjection, WSPosition);
    vout.UV = vertex.UV;
    vout.TBN = TBN;
    vout.ViewDirectionTS = mul(TBNInverse, viewVector);


    vout.Normal = mul(instanceData.NormalMatrix, vertex.Normal);

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
    normal = normal * 2.0 - 1.0;

    return normalize(mul(vertex.TBN, normal));
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

float FetchDisplacementMap(VertexOut vertex, InstanceData instanceData)
{
    Texture2D displacementMap = Textures2D[instanceData.DisplacementMapIndex];
    return displacementMap.Sample(AnisotropicClampSampler, vertex.UV).r;
}

VertexOut DisplaceUV(VertexOut originalVertexData, InstanceData instanceData)
{
    Texture3D distanceField = Textures3D[instanceData.DistanceAtlasIndirectionMapIndex];
    Texture2D displacementMap = Textures2D[instanceData.DisplacementMapIndex];

    float fParallaxLimit = -length(originalVertexData.ViewDirectionTS.xy) / originalVertexData.ViewDirectionTS.z;
    fParallaxLimit *= 0.03;

    float2 vOffsetDir = normalize(originalVertexData.ViewDirectionTS.xy);
    float2 vMaxOffset = vOffsetDir * fParallaxLimit;

    int nNumSamples = 128;
    float fStepSize = 1.0 / (float)nNumSamples;

    float2 dx = ddx(originalVertexData.UV);
    float2 dy = ddy(originalVertexData.UV);

    float fCurrRayHeight = 1.0;
    float2 vCurrOffset = float2(0, 0);
    float2 vLastOffset = float2(0, 0);

    float fLastSampledHeight = 1;
    float fCurrSampledHeight = 1;

    int nCurrSample = 0;

    while (nCurrSample < nNumSamples)
    {
        fCurrSampledHeight = displacementMap.SampleGrad(AnisotropicClampSampler, originalVertexData.UV + vCurrOffset, dx, dy).r;
        if (fCurrSampledHeight > fCurrRayHeight)
        {
            float delta1 = fCurrSampledHeight - fCurrRayHeight;
            float delta2 = (fCurrRayHeight + fStepSize) - fLastSampledHeight;

            float ratio = delta1 / (delta1 + delta2);

            vCurrOffset = (ratio)* vLastOffset + (1.0 - ratio) * vCurrOffset;

            nCurrSample = nNumSamples + 1;
        }
        else
        {
            nCurrSample++;

            fCurrRayHeight -= fStepSize;

            vLastOffset = vCurrOffset;
            vCurrOffset += fStepSize * vMaxOffset;

            fLastSampledHeight = fCurrSampledHeight;
        }
    }

    //originalVertexData.UV = originalVertexData.UV + vCurrOffset;

    return originalVertexData;
}

PixelOut PSMain(VertexOut pin)
{
    InstanceData instanceData = InstanceTable[PassDataCB.InstanceTableIndex];

    VertexOut displacedVertexData = DisplaceUV(pin, instanceData);

    GBufferCookTorrance gBufferData;
    gBufferData.Albedo = pin.Normal;// mul(pin.TBN, float3(0.0, 0.0, 1.0));// FetchAlbedoMap(displacedVertexData, instanceData);
    gBufferData.Normal = FetchNormalMap(displacedVertexData, instanceData);
    gBufferData.Metalness = FetchMetallnessMap(displacedVertexData, instanceData);
    gBufferData.Roughness = FetchRoughnessMap(displacedVertexData, instanceData);
    gBufferData.AO = FetchAOMap(displacedVertexData, instanceData);

    GBufferEncoded encoded = EncodeCookTorranceMaterial(gBufferData);

    PixelOut pixelOut;
    pixelOut.MaterialData = encoded.MaterialData;

    return pixelOut;
}