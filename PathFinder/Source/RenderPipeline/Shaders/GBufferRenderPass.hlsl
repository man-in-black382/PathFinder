struct PassData
{
    uint InstanceTableIndex;
    uint ParallaxCounterTextureUAVIndex;
};

static const float DistanceFieldMaxVoxelDistance = sqrt(3);

struct DistanceFieldCones
{
    float Distances[8];
};

#define PassDataType PassData

#include "InstanceData.hlsl"
#include "BaseRootSignature.hlsl"
#include "GBuffer.hlsl"
#include "ColorConversion.hlsl"
#include "Vertices.hlsl"
#include "Utils.hlsl"

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
};

float3x3 BuildTBNMatrix(Vertex1P1N1UV1T1BT vertex, InstanceData instanceData)
{
    float3 N = mul(instanceData.ModelMatrix, float4(normalize(vertex.Normal), 0.0)).xyz;
    float3 T = mul(instanceData.ModelMatrix, float4(normalize(vertex.Tangent), 0.0)).xyz;
    float3 B = normalize(cross(N, T));

    // Negate B to achieve correct behavior for SOME FUCKING REASON. Shouldn't be needed, but it is.
    return Matrix3x3ColumnMajor(T, B, N);
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

DistanceFieldCones UnpackConeData(uint4 packedCones)
{
    DistanceFieldCones cones;

    float2 c01 = UnpackUnorm2x16(packedCones.x, DistanceFieldMaxVoxelDistance);
    float2 c23 = UnpackUnorm2x16(packedCones.y, DistanceFieldMaxVoxelDistance);
    float2 c45 = UnpackUnorm2x16(packedCones.z, DistanceFieldMaxVoxelDistance);
    float2 c67 = UnpackUnorm2x16(packedCones.w, DistanceFieldMaxVoxelDistance);

    cones.Distances[0] = c01.x;
    cones.Distances[1] = c01.y;
    cones.Distances[2] = c23.x;
    cones.Distances[3] = c23.y;
    cones.Distances[4] = c45.x;
    cones.Distances[5] = c45.y;
    cones.Distances[6] = c67.x;
    cones.Distances[7] = c67.y;

    return cones;
}

uint VectorConeIndex(float3 normalizedVector)
{
    uint index = 0;

    if (abs(normalizedVector.x) > abs(normalizedVector.z))
    {
        index = normalizedVector.x < 0 ? 0 : 2;
    }
    else {
        index = normalizedVector.z < 0 ? 3 : 1;
    }

    if (normalizedVector.y < 0)
    {
        index += 4;
    }

    return index;
}

float3 VoxelIndex(float3 uvw, float3 voxelGridSize)
{
    return uvw * (voxelGridSize - 1.0);
}

VertexOut DisplaceUV(VertexOut originalVertexData, InstanceData instanceData)
{
    Texture3D distanceFieldAtlasIndirectionMap = Textures3D[instanceData.DistanceAtlasIndirectionMapIndex];
    Texture2D displacementMap = Textures2D[instanceData.DisplacementMapIndex];
    Texture3D<uint4> distanceFieldAtlas = UInt4_Textures3D[instanceData.DistanceAtlasIndex];

    float atlasWidth;
    float atlasHeight;
    float atlasDepth;

    distanceFieldAtlas.GetDimensions(atlasWidth, atlasHeight, atlasDepth);

    RWTexture2D<uint4> counterTexture = RW_UInt4_Textures2D[PassDataCB.ParallaxCounterTextureUAVIndex];

    float3 samplingPosition = float3(originalVertexData.UV, 1.0); 
    float3 voxelIndex = VoxelIndex(samplingPosition, float3(atlasWidth, atlasHeight, atlasDepth));
    float3 viewDir = originalVertexData.ViewDirectionTS;

    // Scaling up Z is equivalent to scaling down values in the displacement map.
    float POMScale = 8.0;

    viewDir.z *= POMScale;
    viewDir = normalize(viewDir);

    uint viewDirConeIndex = 0;// VectorConeIndex(-viewDir);
    DistanceFieldCones cones = UnpackConeData(distanceFieldAtlas.Load(int4(voxelIndex, 0)));
    float distance = cones.Distances[viewDirConeIndex];

    uint i = 0;

    while (distance > 0.0)
    {
        samplingPosition -= viewDir * distance;
        voxelIndex = VoxelIndex(samplingPosition, float3(atlasWidth, atlasHeight, atlasDepth));
        cones = UnpackConeData(distanceFieldAtlas.Load(int4(voxelIndex, 0)));
        distance = cones.Distances[viewDirConeIndex];

        // Sanity check
        if (++i > 150)
        {
            break;
        }
    }

    counterTexture[originalVertexData.Position.xy].r = i;
    originalVertexData.UV = samplingPosition.xy;
    return originalVertexData;
}

PixelOut PSMain(VertexOut pin)
{
    InstanceData instanceData = InstanceTable[PassDataCB.InstanceTableIndex];

    VertexOut displacedVertexData = DisplaceUV(pin, instanceData);

    GBufferCookTorrance gBufferData;

    gBufferData.Albedo = FetchAlbedoMap(displacedVertexData, instanceData);
    gBufferData.Normal = FetchNormalMap(displacedVertexData, instanceData);
    gBufferData.Metalness = FetchMetallnessMap(displacedVertexData, instanceData);
    gBufferData.Roughness = FetchRoughnessMap(displacedVertexData, instanceData);
    gBufferData.AO = FetchDisplacementMap(displacedVertexData, instanceData);//  FetchAOMap(displacedVertexData, instanceData);

    GBufferEncoded encoded = EncodeCookTorranceMaterial(gBufferData);

    PixelOut pixelOut;
    pixelOut.MaterialData = encoded.MaterialData;

    return pixelOut;
}