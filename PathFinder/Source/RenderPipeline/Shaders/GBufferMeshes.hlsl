#ifndef _GBufferMeshes__
#define _GBufferMeshes__

struct RootConstants
{
    uint InstanceTableIndex;
};

#include "MandatoryEntryPointInclude.hlsl"
#include "GBuffer.hlsl"
#include "ColorConversion.hlsl"
#include "Vertices.hlsl"
#include "Geometry.hlsl"
#include "Matrix.hlsl"
#include "Mesh.hlsl"
#include "Utils.hlsl"

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);
StructuredBuffer<Vertex1P1N1UV1T1BT> UnifiedVertexBuffer : register(t0);
StructuredBuffer<uint> UnifiedIndexBuffer : register(t1);
StructuredBuffer<MeshInstance> InstanceTable : register(t2);
StructuredBuffer<Material> MaterialTable : register(t3);

//------------------------  Vertex  ------------------------------//

struct VertexOut
{
    float4 Position : SV_POSITION;
    float4 PreviousPosition : PREVIOUS_POSITION;
    float ViewDepth : VIEW_DEPTH;
    float2 UV : TEXCOORD0;  
    float3x3 TBN : TBN_MATRIX;
};

float3x3 BuildTBNMatrix(Vertex1P1N1UV1T1BT vertex, MeshInstance instanceData)
{
    float3 N = normalize(mul(instanceData.NormalMatrix, float4(normalize(vertex.Normal), 0.0)).xyz);
    float3 T = normalize(mul(instanceData.NormalMatrix, float4(normalize(vertex.Tangent), 0.0)).xyz);
    float3 B = cross(N, T);

    return Matrix3x3ColumnMajor(T, B, N);
}

VertexOut VSMain(uint indexId : SV_VertexID)
{
    VertexOut vout;
    
    MeshInstance instanceData = InstanceTable[RootConstantBuffer.InstanceTableIndex];

    // Load index and vertex
    uint index = UnifiedIndexBuffer[instanceData.UnifiedIndexBufferOffset + indexId];
    Vertex1P1N1UV1T1BT vertex = UnifiedVertexBuffer[instanceData.UnifiedVertexBufferOffset + index];

    float3x3 TBN = BuildTBNMatrix(vertex, instanceData);

    float4 WSPosition = mul(instanceData.ModelMatrix, vertex.Position);
    float4 prevWSPosition = mul(instanceData.PrevModelMatrix, vertex.Position);
    float4 CSPosition = mul(FrameDataCB.CurrentFrameCamera.View, WSPosition);

    vout.Position = mul(FrameDataCB.CurrentFrameCamera.ViewProjectionJitter, WSPosition);
    // We don't want jitter in motion vectors, so project previous position without it
    vout.PreviousPosition = mul(FrameDataCB.PreviousFrameCamera.ViewProjection, prevWSPosition);
    vout.ViewDepth = CSPosition.z;
    vout.UV = vertex.UV;
    vout.TBN = TBN;

    return vout;
}

//------------------------  Pixel  ------------------------------//

float3 FetchAlbedoMap(VertexOut vertex, Material material, SamplerState sampler)
{
    if (all(material.DiffuseAlbedoOverride < 0.0))
    {
        Texture2D albedoMap = Textures2D[material.AlbedoMapIndex];
        return SRGBToLinear(albedoMap.Sample(sampler, vertex.UV).rgb);
    }
    else
    {
        return material.DiffuseAlbedoOverride;
    }
}

float3 FetchNormalMap(VertexOut vertex, Material material, SamplerState sampler)
{
    Texture2D normalMap = Textures2D[material.NormalMapIndex];
    
    float3 normal = normalMap.Sample(sampler, vertex.UV).xyz;
    normal = normal * 2.0 - 1.0;

    return normalize(mul(vertex.TBN, normal));
}

float FetchMetallnessMap(VertexOut vertex, Material material, SamplerState sampler)
{
    if (material.MetalnessOverride < 0.0)
    {
        Texture2D metalnessMap = Textures2D[material.MetalnessMapIndex];
        return metalnessMap.Sample(sampler, vertex.UV).r;
    }
    else
    {
        return material.MetalnessOverride;
    }
}

float FetchRoughnessMap(VertexOut vertex, Material material, SamplerState sampler)
{
    if (material.RoughnessOverride < 0.0)
    {
        Texture2D roughnessMap = Textures2D[material.RoughnessMapIndex];
        return roughnessMap.Sample(sampler, vertex.UV).r;
    }
    else
    {
        return material.RoughnessOverride;
    }
}

float FetchDisplacementMap(VertexOut vertex, Material material, SamplerState sampler)
{
    Texture2D displacementMap = Textures2D[material.DisplacementMapIndex];
    return displacementMap.Sample(sampler, vertex.UV).r;
}

GBufferPixelOut PSMain(VertexOut pin) 
{
    MeshInstance instanceData = InstanceTable[RootConstantBuffer.InstanceTableIndex];
    Material material = MaterialTable[instanceData.MaterialIndex];

    /*if (material.TranslucencyOverride > 0.0)
    {
        discard;
    }*/

    SamplerState sampler = Samplers[material.SamplerIndex];

    float3 normal = instanceData.HasTangentSpace && material.HasNormalMap ?
        FetchNormalMap(pin, material, sampler) :
        normalize(float3(pin.TBN[0][2], pin.TBN[1][2], pin.TBN[2][2]));

    float2 jitter = FrameDataCB.CurrentFrameCamera.UVJitter;
    float3 previousNDCPosition = pin.PreviousPosition.xyz / pin.PreviousPosition.w;
    float2 previousScreenUV = NDCToUV(previousNDCPosition);
    previousScreenUV += jitter; // Get rid of the jitter caused by perspective interpolation with W from jittered matrix
    float3 previousUVSpacePosition = float3(previousScreenUV, previousNDCPosition.z);

    float2 currentScreenUV = (floor(pin.Position.xy) + 0.5) * GlobalDataCB.PipelineRTResolutionInv;
    float3 currentUVSpacePosition = float3(currentScreenUV, pin.Position.z);

    float3 velocity = currentUVSpacePosition - previousUVSpacePosition;

    GBufferPixelOut pixelOut = GetStandardGBufferPixelOutput(
        FetchAlbedoMap(pin, material, sampler),
        FetchMetallnessMap(pin, material, sampler),
        FetchRoughnessMap(pin, material, sampler),
        normal,
        velocity,
        instanceData.MaterialIndex,
        pin.ViewDepth
    );

    return pixelOut;
}

#endif