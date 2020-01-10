static const float DistanceFieldMaxVoxelDistance = sqrt(3.0);

struct DistanceFieldCones
{
    float Distances[8];
};

struct SamplingCorners
{
    float2 UV0;
    float2 UV1;
    float2 UV2;
    float2 UV3;
};

struct RootConstants
{
    uint InstanceTableIndex;
};

#include "InstanceData.hlsl"
#include "MandatoryEntryPointInclude.hlsl"
#include "GBuffer.hlsl"
#include "ColorConversion.hlsl"
#include "Vertices.hlsl"
#include "Geometry.hlsl"

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);
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
    
    InstanceData instanceData = InstanceTable[RootConstantBuffer.InstanceTableIndex];

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

SamplingCorners BilinearPatchCorners(float2 cornerUV, float2 displacementMapSize, float3 voxelGridSize)
{
    float2 texelCountPerVoxel = CountFittingTexels(displacementMapSize, voxelGridSize.xy);
    
    // Add 1 here to sample 1 texel more just to make ray - patch intersection more reliable
    // This is purely based on debugging results
    float2 uvIncrement = (texelCountPerVoxel) / displacementMapSize; 

    SamplingCorners corners;

    cornerUV -= 1.0 / 64.0;
    uvIncrement += 2.0 / 64.0;

    corners.UV0 = cornerUV;
    corners.UV1 = float2(cornerUV.x, cornerUV.y + uvIncrement.y);
    corners.UV2 = float2(cornerUV.x + uvIncrement.x, cornerUV.y + uvIncrement.y);
    corners.UV3 = float2(cornerUV.x + uvIncrement.x, cornerUV.y);
    return corners;
}

VertexOut DisplaceUV(VertexOut originalVertexData, InstanceData instanceData, out bool patchIntersects, out float3 voxel)
{
    Texture3D distanceFieldAtlasIndirectionMap = Textures3D[instanceData.DistanceAtlasIndirectionMapIndex];
    Texture2D displacementMap = Textures2D[instanceData.DisplacementMapIndex];
    Texture3D<uint4> distanceFieldAtlas = UInt4_Textures3D[instanceData.DistanceAtlasIndex];

    uint atlasWidth;
    uint atlasHeight;
    uint atlasDepth;

    distanceFieldAtlas.GetDimensions(atlasWidth, atlasHeight, atlasDepth);

    uint displacementMapWidth;
    uint displacementMapsHeight;

    displacementMap.GetDimensions(displacementMapWidth, displacementMapsHeight);

    uint2 displacementMapSize = uint2(displacementMapWidth, displacementMapsHeight);
    uint3 atlasSize = uint3(atlasWidth, atlasHeight, atlasDepth);

    patchIntersects = true;

    // Scaling up Z is equivalent to scaling down values in the displacement map.
    float POMScale = 20.0;

    float3 viewDir = originalVertexData.ViewDirectionTS;
    viewDir.z *= POMScale;
    viewDir = normalize(viewDir);

    //viewDir = float3(0.0, 0.0, 1.0);
    
    Ray traversalRay = { float3(originalVertexData.UV, 1.0), -viewDir, 0.0, DistanceFieldMaxVoxelDistance };
    float3 samplingPosition = traversalRay.Origin;
    uint viewDirConeIndex = VectorOctant(traversalRay.Direction);
   
    static const uint MaxStepCount = 16;

    for (uint i = 0; i < MaxStepCount; ++i)
    {
        uint3 voxelIndex = UVWToVoxelIndex(samplingPosition, atlasSize);
        DistanceFieldCones cones = UnpackConeData(distanceFieldAtlas.Load(int4(voxelIndex, 0)));
        float safeTravelDistance = cones.Distances[viewDirConeIndex];
        bool voxelIntersectedByDisplacementSurface = safeTravelDistance <= 0.0;

        voxel = voxelIndex;

        if (voxelIntersectedByDisplacementSurface)
        {
            // Construct a bilinear patch for interpolation
            float3 voxelUVW = VoxelIndexToUVW(voxelIndex, atlasSize);
            SamplingCorners bpCorners = BilinearPatchCorners(voxelUVW.xy, displacementMapSize, atlasSize);

            float3 pb0 = float3(bpCorners.UV0, displacementMap.SampleLevel(PointClampSampler, bpCorners.UV0, 0).r);
            float3 pb1 = float3(bpCorners.UV1, displacementMap.SampleLevel(PointClampSampler, bpCorners.UV1, 0).r);
            float3 pb2 = float3(bpCorners.UV2, displacementMap.SampleLevel(PointClampSampler, bpCorners.UV2, 0).r);
            float3 pb3 = float3(bpCorners.UV3, displacementMap.SampleLevel(PointClampSampler, bpCorners.UV3, 0).r);

            BilinearPatch patch = { pb0, pb1, pb2, pb3 };
            
            float3 intersectionPoint;
            if (IntersectPatch(patch, traversalRay, intersectionPoint))
            {
                // If we hit a patch, then we're done
                originalVertexData.UV = intersectionPoint.xy;
                return originalVertexData;
            }
            else
            {
                //return originalVertexData;
                // Missed the patch. Go to the next voxel.
                samplingPosition = VoxelWallIntersection(samplingPosition, atlasSize, traversalRay);

                voxelIndex = UVWToVoxelIndex(samplingPosition, atlasSize);
                samplingPosition = VoxelIndexToUVW(voxelIndex, atlasSize);
            }
        }
        else
        {
            samplingPosition += traversalRay.Direction * safeTravelDistance;
        }
    }

    originalVertexData.UV = samplingPosition.xy;
    
    return originalVertexData;
}

PixelOut PSMain(VertexOut pin)
{
    InstanceData instanceData = InstanceTable[RootConstantBuffer.InstanceTableIndex];

    bool intersection;
    float3 voxel;
    VertexOut displacedVertexData = DisplaceUV(pin, instanceData, intersection, voxel);

    GBufferCookTorrance gBufferData;

    gBufferData.Albedo = FetchAlbedoMap(displacedVertexData, instanceData);
    gBufferData.Normal = FetchNormalMap(displacedVertexData, instanceData);
    gBufferData.Metalness = FetchMetallnessMap(displacedVertexData, instanceData);
    gBufferData.Roughness = FetchRoughnessMap(displacedVertexData, instanceData);
    gBufferData.AO = FetchAOMap(displacedVertexData, instanceData);

    gBufferData.Albedo = intersection ? FetchDisplacementMap(displacedVertexData, instanceData).xxx : 0.0;

   /* if (voxel.x == 26 && voxel.y == 36)
    {
        gBufferData.Albedo = float3(0.0, 0.0, 1.0);
    }
    else
    {
        gBufferData.Albedo = float3(0.0, voxel.z / 64, 0.0);
    }*/

    //Ray testRay = { float3(0.5, 0.5, 0.5), float3(-1, -1, 0), 0, 0 };
    //float3 newPos = VoxelWallIntersection(testRay.Origin, float3(5, 5, 5), testRay);
    //uint3 i = UVWToVoxelIndex(newPos, uint3(5, 5, 5));

    //if (all(i == uint3(1, 1, 2)))
    //{
    //    gBufferData.Albedo = float3(0.0, voxel.z / 64, 0.0);
    //}

    GBufferEncoded encoded = EncodeCookTorranceMaterial(gBufferData);

    PixelOut pixelOut;
    pixelOut.MaterialData = encoded.MaterialData;

    DebugOut(123.0, pin.Position.xy, uint2(1000, 500));
    DebugOut(566.0, pin.Position.xy, uint2(1000, 500));
    DebugOut(2548.1234, pin.Position.xy, uint2(1000, 500));

    return pixelOut;
}