#ifndef _GIProbeHelpers__
#define _GIProbeHelpers__

#include "Random.hlsl"
#include "Utils.hlsl"
#include "Packing.hlsl"
#include "Geometry.hlsl"

static const float ProbeIrradianceGamma = 5.0;
static const float ProbeRayBackfaceIndicator = -1.0;

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
    uint CurrentIrradianceProbeAtlasTexIdx;
    uint CurrentDepthProbeAtlasTexIdx;
    // 16 byte boundary
    // How many probe planes were spawned this frame on each axis.
    // Can be negative if probes are spawned along negative axis direction.
    int3 SpawnedProbePlanesCount; 
    float DebugProbeRadius;
    // 16 byte boundary
    uint PreviousIrradianceProbeAtlasTexIdx;
    uint PreviousDepthProbeAtlasTexIdx;
    float IrradianceHysteresisDecrease;
    float DepthHysteresisDecrease;
};

uint3 Probe3DIndexFrom1D(uint index, IrradianceField field)
{
    uint3 index3d;

    /* Works for any # of probes */
    index3d.x = index % field.GridSize.x;
    index3d.y = (index % (field.GridSize.x * field.GridSize.y)) / field.GridSize.x;
    index3d.z = index / (field.GridSize.x * field.GridSize.y);

    // Assumes probe counts are powers of two.
    // Saves time compared to the divisions above
    // Precomputing the MSB actually slows this code down substantially
   /* 
    index3d.x = index & (field.GridSize.x - 1);
    index3d.y = (index & ((field.GridSize.x * field.GridSize.y) - 1)) >> FindMSB(field.GridSize.x);
    index3d.z = index >> FindMSB(field.GridSize.x * field.GridSize.y);*/

    return index3d;
}

uint Probe1DIndexFrom3D(uint3 index, IrradianceField field)
{
    return index.x + index.y * field.GridSize.x + index.z * field.GridSize.x * field.GridSize.y;
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

// Compute normalized oct coords, mapping top left of top left pixel to (-1,-1)
float2 NormalizedOctCoordFromTexelIndex(uint2 probeTexelIndex, uint probeSize)
{
    // Add back the half pixel to get pixel center normalized coordinates
    return (float2(probeTexelIndex) + 0.5) * (2.0 / float(probeSize)) - 1.0;
}

float2 NormalizedIrradianceProbeOctCoord(uint2 probeTexelIndex, IrradianceField field)
{
    return NormalizedOctCoordFromTexelIndex(probeTexelIndex, field.IrradianceProbeSize);
}

float2 NormalizedDepthProbeOctCoord(uint2 probeTexelIndex, IrradianceField field)
{
    return NormalizedOctCoordFromTexelIndex(probeTexelIndex, field.DepthProbeSize);
}

float3 EncodeProbeIrradiance(float3 irradiance)
{
    return pow(irradiance, 1.0 / ProbeIrradianceGamma);
}

float3 DecodeProbeIrradiance(float3 encoded)
{
    return pow(encoded, ProbeIrradianceGamma);
}

float3 RetrieveGIIrradiance(
    float3 surfacePosition,
    float3 surfaceNormal, 
    float3 viewDirection,
    Texture2D irradianceProbeAtlas,
    Texture2D depthProbeAtlas,
    SamplerState sampler,
    IrradianceField field,
    bool samplingLastFrame)
{
    const float D = field.CellSize;
    const float B = 0.3;
    float3 selfShadowBias = (surfaceNormal * 0.2 + viewDirection * 0.8) * (0.75 * D) * B;

    float3 trueSurfacePosition = surfacePosition;
    surfacePosition += selfShadowBias;

    float3 firstProbe3DIndex = (surfacePosition - field.GridCornerPosition) / field.CellSize;
    // alpha is how far from the floor(currentVertex) position. on [0, 1] for each axis.
    float3 alpha = frac(firstProbe3DIndex);
    firstProbe3DIndex = floor(firstProbe3DIndex);

    //        5-------6
    //       /|      /|
    // Y    / |     / |
    // ^   1--|----2  |
    // |   |  4----|--7
    // |   | /     | /
    // |   0-------3
    // |
    //  -----------> X
    static const uint3 IndexOffsets[8] = {
        uint3(0,0,0), uint3(0,1,0), uint3(1,1,0), uint3(1,0,0),
        uint3(0,0,1), uint3(0,1,1), uint3(1,1,1), uint3(1,0,1)
    };
    
    float3 irradiance = 0.0;
    float totalWeight = 0.0;

    // If we're sampling last frame data, we need to adjust 3D indices
    int3 index3DLastFrameCorrection = samplingLastFrame ? field.SpawnedProbePlanesCount : 0;

    for (uint i = 0; i < 8; ++i)
    {
        uint3 probe3DIndex = clamp(firstProbe3DIndex + IndexOffsets[i] + index3DLastFrameCorrection, 0, field.GridSize - 1);
        uint probeIndex = Probe1DIndexFrom3D(probe3DIndex, field);
        float3 probePosition = ProbePositionFrom3DIndex(probe3DIndex, field);
        float3 surfaceToProbe = probePosition - surfacePosition;
        float distToProbe = length(surfaceToProbe);
        surfaceToProbe /= distToProbe;

        float weight = 1.0;

        {
            //Smooth backface
            
            // Computed without the biasing applied to the "dir" variable. 
            // This test can cause reflection-map looking errors in the image
            // (stuff looks shiny) if the transition is poor.
            float3 trueDirectionToProbe = normalize(probePosition - trueSurfacePosition);

            // The naive soft backface weight would ignore a probe when
            // it is behind the surface. That's good for walls. But for small details inside of a
            // room, the normals on the details might rule out all of the probes that have mutual
            // visibility to the point. So, we instead use a "wrap shading" test below inspired by
            // NPR work.
            // weight *= max(0.0001, dot(trueDirectionToProbe, wsN));

            // The small offset at the end reduces the "going to zero" impact
            // where this is really close to exactly opposite
            float backfaceWeight = Square(max(0.0001, (dot(trueDirectionToProbe, surfaceNormal) + 1.0) * 0.5)) + 0.2;

            weight *= backfaceWeight;
        }
        
        {
            // Visibility (Chebyshev)
            float3 probeToSurface = -surfaceToProbe;
            float2 depthAtlasUV = DepthProbeAtlasUV(probeIndex, probeToSurface, field);
            float2 meanAndMean2 = depthProbeAtlas.SampleLevel(sampler, depthAtlasUV, 0).rg;
            float mean = meanAndMean2.x;
            float mean2 = meanAndMean2.y;
            float variance = abs(Square(mean) - mean2);

            // http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
            // Need the max in the denominator because biasing can cause a negative displacement
            float chebyshevWeight = variance / (variance + Square(max(distToProbe - mean, 0.0)));  

            // Increase contrast in the weight 
            chebyshevWeight = max(Cube(chebyshevWeight), 0.0);
            weight *= (distToProbe <= mean) ? 1.0 : chebyshevWeight;
        }

        {
            // A tiny bit of light is really visible due to log perception, so
            // crush tiny weights but keep the curve continuous. This must be done
            // before the trilinear weights, because those should be preserved.
            const float CrushThreshold = 0.2;
            if (weight < CrushThreshold) 
                weight *= Square(weight) / Square(CrushThreshold);
        }

        float2 irradianceAtlasUV = IrradianceProbeAtlasUV(probeIndex, surfaceNormal, field);
        float4 probeIrradianceAndBackfaceWeight = irradianceProbeAtlas.SampleLevel(sampler, irradianceAtlasUV, 0);
        float3 probeIrradiance = probeIrradianceAndBackfaceWeight.rgb;
        float backfaceWeight = probeIrradianceAndBackfaceWeight.w;

        {
            // If probe rays were hitting geometry back faces, this weight will 
            // bring weight of those probes down completely disabling probes that are inside walls.
            weight *= backfaceWeight;
        }

        {
            // Compute the trilinear weights based on the grid cell vertex to smoothly
            // transition between probes. Avoid ever going entirely to zero because that
            // will cause problems at the border probes. This isn't really a lerp. 
            // We're using 1-a when offset = 0 and a when offset = 1.
            float3 trilinear = lerp(1.0 - alpha, alpha, IndexOffsets[i]);
            weight *= trilinear.x * trilinear.y * trilinear.z;
        }

        // Decode the tone curve, but leave a gamma = 2 curve
        // to approximate sRGB blending for the trilinear
        probeIrradiance = pow(probeIrradiance, ProbeIrradianceGamma * 0.5);

        irradiance += probeIrradiance * weight;
        totalWeight += weight;
    }

    // Avoid NaNs
    totalWeight = max(totalWeight, 0.0001);

    // Go back to linear irradiance
    irradiance = Square(irradiance / totalWeight);

    return irradiance;
}

#endif