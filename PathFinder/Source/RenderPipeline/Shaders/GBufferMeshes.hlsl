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

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);
StructuredBuffer<Vertex1P1N1UV1T1BT> UnifiedVertexBuffer : register(t0);
StructuredBuffer<IndexU32> UnifiedIndexBuffer : register(t1);
StructuredBuffer<MeshInstance> InstanceTable : register(t2);
StructuredBuffer<Material> MaterialTable : register(t3);

//------------------------  Vertex  ------------------------------//

struct VertexOut
{
    float4 Position : SV_POSITION;
    float ViewDepth : VIEW_DEPTH;
    float2 UV : TEXCOORD0;  
    float3x3 TBN : TBN_MATRIX;
};

float3x3 BuildTBNMatrix(Vertex1P1N1UV1T1BT vertex, MeshInstance instanceData)
{
    float3 N = mul(instanceData.ModelMatrix, float4(normalize(vertex.Normal), 0.0)).xyz;
    float3 T = mul(instanceData.ModelMatrix, float4(normalize(vertex.Tangent), 0.0)).xyz;
    float3 B = normalize(cross(N, T));

    return Matrix3x3ColumnMajor(T, B, N);
}

VertexOut VSMain(uint indexId : SV_VertexID)
{
    VertexOut vout;
    
    MeshInstance instanceData = InstanceTable[RootConstantBuffer.InstanceTableIndex];

    // Load index and vertex
    IndexU32 index = UnifiedIndexBuffer[instanceData.UnifiedIndexBufferOffset + indexId];
    Vertex1P1N1UV1T1BT vertex = UnifiedVertexBuffer[instanceData.UnifiedVertexBufferOffset + index.Index];

    float3x3 TBN = BuildTBNMatrix(vertex, instanceData);
    float3x3 TBNInverse = transpose(TBN);

    float4 WSPosition = mul(instanceData.ModelMatrix, vertex.Position);
    float4 CSPosition = mul(FrameDataCB.CurrentFrameCamera.View, WSPosition);
    float4 ClipSPosition = mul(FrameDataCB.CurrentFrameCamera.Projection, CSPosition);

    vout.Position = ClipSPosition;
    vout.ViewDepth = CSPosition.z;
    vout.UV = vertex.UV;
    vout.TBN = TBN;

    return vout;
}

//------------------------  Pixel  ------------------------------//

float3 FetchAlbedoMap(VertexOut vertex, Material material)
{
    return 0.8;//

    Texture2D albedoMap = Textures2D[material.AlbedoMapIndex];
    return LinearFromSRGB(albedoMap.Sample(AnisotropicClampSampler, vertex.UV).rgb);
}

float3 FetchNormalMap(VertexOut vertex, Material material)
{
    Texture2D normalMap = Textures2D[material.NormalMapIndex];
    
    float3 normal = normalMap.Sample(AnisotropicClampSampler, vertex.UV).xyz; 
    normal = normal * 2.0 - 1.0;

    normal = float3(0, 0, 1); 

    return normalize(mul(vertex.TBN, normal));
}

float FetchMetallnessMap(VertexOut vertex, Material material)
{
    return 0.0;//

    Texture2D metalnessMap = Textures2D[material.MetalnessMapIndex];
    return metalnessMap.Sample(AnisotropicClampSampler, vertex.UV).r; 
}

float FetchRoughnessMap(VertexOut vertex, Material material)
{
    return 0.1;//

    Texture2D roughnessMap = Textures2D[material.RoughnessMapIndex];
    return roughnessMap.Sample(AnisotropicClampSampler, vertex.UV).r;
}

float FetchAOMap(VertexOut vertex, Material material)
{
    Texture2D aoMap = Textures2D[material.AOMapIndex];
    return aoMap.Sample(AnisotropicClampSampler, vertex.UV).r;
}

float FetchDisplacementMap(VertexOut vertex, Material material)
{
    Texture2D displacementMap = Textures2D[material.DisplacementMapIndex];
    return displacementMap.Sample(AnisotropicClampSampler, vertex.UV).r;
}

GBufferPixelOut PSMain(VertexOut pin)
{
    MeshInstance instanceData = InstanceTable[RootConstantBuffer.InstanceTableIndex];
    Material material = MaterialTable[instanceData.MaterialIndex];

    GBufferPixelOut pixelOut = GetStandardGBufferPixelOutput(
        FetchAlbedoMap(pin, material),
        FetchMetallnessMap(pin, material),
        FetchRoughnessMap(pin, material),
        FetchNormalMap(pin, material),
        0.0,
        instanceData.MaterialIndex,
        pin.ViewDepth
    );

    //DebugOut(123.0, pin.Position.xy, uint2(1000, 500));
    //DebugOut(566.0, pin.Position.xy, uint2(1000, 500));
    //DebugOut(2548.1234, pin.Position.xy, uint2(1000, 500));

    return pixelOut;
}

#endif