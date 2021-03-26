#ifndef _GIProbeHelpers__
#define _GIProbeHelpers__

#include "Random.hlsl"
#include "Utils.hlsl"
#include "Packing.hlsl"
#include "Geometry.hlsl"

static const float GITRayMiss = -1.0;

struct IrradianceField
{
    uint3 GridSize;
    float CellSize;
    // 16 byte boundary
    float3 GridCornerPosition;
    uint RaysPerProbe;
    // 16 byte boundary
    uint TotalProbeCount;
    uint2 RayHitInfoTextureSize;
    uint RayHitInfoTextureIdx;
    // 16 byte boundary
    float4x4 ProbeRotation;
    // 16 byte boundary
    uint2 IrradianceProbeAtlasSize;
    uint2 DepthProbeAtlasSize;
    // 16 byte boundary
    uint2 IrradianceProbeAtlasProbesPerDimension;
    uint2 DepthProbeAtlasProbesPerDimension;
    // 16 byte boundary
    uint IrradianceProbeSize;
    uint DepthProbeSize;
    uint IrradianceProbeAtlasTexIdx;
    uint DepthProbeAtlasTexIdx;
    // 16 byte boundary
    // How many probe planes were spawned this frame on each axis.
    // Can be negative if probes are spawned along negative axis direction.
    int3 SpawnedProbePlanesCount; 
    uint IndirectionTableTexIdx;
    // 16 byte boundary
    float DebugProbeRadius;
    uint Pad0__;
    uint Pad1__;
    uint Pad2__;
};

uint3 Probe3DIndexFrom1D(uint index, IrradianceField field)
{
    /* Works for any # of probes */
    /*
    iPos.x = index % L.probeCounts.x;
    iPos.y = (index % (L.probeCounts.x * L.probeCounts.y)) / L.probeCounts.x;
    iPos.z = index / (L.probeCounts.x * L.probeCounts.y);
    */

    // Assumes probe counts are powers of two.
    // Saves ~10ms compared to the divisions above
    // Precomputing the MSB actually slows this code down substantially
    uint3 index3d;
    index3d.x = index & (field.GridSize.x - 1);
    index3d.y = (index & ((field.GridSize.x * field.GridSize.y) - 1)) >> FindMSB(field.GridSize.x);
    index3d.z = index >> FindMSB(field.GridSize.x * field.GridSize.y);

    return index3d;
}

uint Probe1DIndexFromRayIndex(uint rayIndex, IrradianceField field)
{
    return rayIndex / field.RaysPerProbe;
}

float3 ProbePositionFrom3DIndex(uint3 index, IrradianceField field)
{
    return field.CellSize * index + field.GridCornerPosition;
}

uint2 RayHitTexelIndex(uint rayIndex, uint probeIndex, IrradianceField field)
{
    return uint2(probeIndex, rayIndex % field.RaysPerProbe);
}

float3 ProbeSamplingVector(uint rayIndex, IrradianceField field)
{
    float3 v = SphericalFibonacci(rayIndex % field.RaysPerProbe, field.RaysPerProbe);
    return mul(field.ProbeRotation, float4(v, 0.0)).xyz;
}

float3 ProbePositionFromRayIndex(uint rayIndex, IrradianceField field)
{
    uint probeIndex = Probe1DIndexFromRayIndex(rayIndex, field);
    uint3 probe3DIndex = Probe3DIndexFrom1D(probeIndex, field);
    float3 probePosition = ProbePositionFrom3DIndex(probe3DIndex, field);

    return probePosition;
}

uint2 ProbeAtlasTexelIndex(uint probeIndex, uint2 probeLocalTexelIndex, uint probeSize, uint2 probesPerDimension)
{
    uint probeSizeWithBorders = probeSize + 2;
    uint2 indexInAtlas = Index2DFrom1D(probeIndex, probesPerDimension);
    uint2 cornerTexel = indexInAtlas * probeSizeWithBorders;
    uint2 actualTexel = cornerTexel + 1 + probeLocalTexelIndex; // +1 to move out from the corner

    return actualTexel;
}

uint2 IrradianceProbeAtlasTexelIndex(uint probeIndex, uint2 probeLocalTexelIndex, IrradianceField field)
{
    return ProbeAtlasTexelIndex(probeIndex, probeLocalTexelIndex, field.IrradianceProbeSize, field.IrradianceProbeAtlasProbesPerDimension);
}

uint2 DepthProbeAtlasTexelIndex(uint probeIndex, uint2 probeLocalTexelIndex, IrradianceField field)
{
    return ProbeAtlasTexelIndex(probeIndex, probeLocalTexelIndex, field.DepthProbeSize, field.DepthProbeAtlasProbesPerDimension);
}

float2 ProbeAtlasUV(uint probeIndex, float3 samplingDir, uint probeSize, uint2 probesPerDimension, uint2 atlasTexSize)
{
    float2 localUV = (OctEncode(samplingDir) + 1.0) * 0.5;
    uint probeSizeWithBorders = probeSize + 2;
    uint2 indexInAtlas = Index2DFrom1D(probeIndex, probesPerDimension);
    uint2 cornerTexel = indexInAtlas * probeSizeWithBorders;
    float2 probeStartUVInAtlas = float2(cornerTexel + 1.0) / atlasTexSize;
    float2 probeUVSizeInAtlasNoBorders = float(probeSize) / atlasTexSize; 

    return probeStartUVInAtlas + localUV * probeUVSizeInAtlasNoBorders;
}

float2 IrradianceProbeAtlasUV(uint probeIndex, float3 samplingDir, IrradianceField field)
{
    return ProbeAtlasUV(probeIndex, samplingDir, field.IrradianceProbeSize, field.IrradianceProbeAtlasProbesPerDimension, field.IrradianceProbeAtlasSize);
}

float2 DepthProbeAtlasUV(uint probeIndex, float3 samplingDir, IrradianceField field)
{
    return ProbeAtlasUV(probeIndex, samplingDir, field.DepthProbeSize, field.DepthProbeAtlasProbesPerDimension, field.DepthProbeAtlasSize);
}

// Compute normalized oct coord, mapping top left of top left pixel to (-1,-1)
float2 NormalizedOctCoord(uint2 probeTexelIndex, uint probeSize)
{
    // Add back the half pixel to get pixel center normalized coordinates
    return (float2(probeTexelIndex) + 0.5) * (2.0 / float(probeSize)) - 1.0;
}

float2 NormalizedIrradianceProbeOctCoord(uint2 probeTexelIndex, IrradianceField field)
{
    return NormalizedOctCoord(probeTexelIndex, field.IrradianceProbeSize);
}

float2 NormalizedDepthProbeOctCoord(uint2 probeTexelIndex, IrradianceField field)
{
    return NormalizedOctCoord(probeTexelIndex, field.DepthProbeSize);
}

float3 EncodeProbeIrradiance(float3 irradiance)
{
    // Perceptual encoding
    const float GammaInv = 1.0 / 5.0;

    return pow(irradiance, GammaInv);
}

float3 DecodeProbeIrradiance(float3 encoded)
{
    const float Gamma = 5.0;

    return pow(encoded, Gamma);
}

#endif