#ifndef _DenoiserHistoryFix__
#define _DenoiserHistoryFix__

#include "GroupsharedMemoryHelpers.hlsl"
#include "Random.hlsl"
#include "ColorConversion.hlsl"
#include "ThreadGroupTilingX.hlsl"
#include "Geometry.hlsl"
#include "GBuffer.hlsl"

struct PassData
{
    uint2 DispatchGroupCount;
    uint PreviousFrameTexIdx;
    uint CurrentFrameTexIdx;
    uint MotionTexIdx;
    uint OutputTexIdx;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"

static const int GroupDimensionSize = 16;
static const int SamplingRadius = 1;
static const int GSArrayDimensionSize = GroupDimensionSize + 2 * SamplingRadius;
static const float TAASampleCount = 16.0;

groupshared min16float3 gCache[GSArrayDimensionSize][GSArrayDimensionSize];

// Here the ray referenced goes from history to (filtered) center color
float DistToAABB(float3 color, float3 history, float3 minimum, float3 maximum) 
{
    float3 center = 0.5 * (maximum + minimum);
    float3 extents = 0.5 * (maximum - minimum);

    float3 rayDir = color - history;
    float3 rayPos = history - center;

    float3 invDir = rcp(rayDir);
    float3 t0 = (extents - rayPos) * invDir;
    float3 t1 = -(extents + rayPos) * invDir;

    float AABBIntersection = max(max(min(t0.x, t1.x), min(t0.y, t1.y)), min(t0.z, t1.z));
    return saturate(AABBIntersection);
}

// From Playdead's TAA
float3 DirectClipToAABB(float3 history, float3 minimum, float3 maximum)
{
    // note: only clips towards aabb center (but fast!)
    float3 center = 0.5 * (maximum + minimum);
    float3 extents = 0.5 * (maximum - minimum);

    // This is actually `distance`, however the keyword is reserved
    float3 offset = history - center;
    float3 v_unit = offset.xyz / extents.xyz;
    float3 absUnit = abs(v_unit);
    float maxUnit = max(absUnit.x, max(absUnit.y, absUnit.z));

    if (maxUnit > 1.0)
        return center + (offset / maxUnit);
    else
        return history;
}

// ---------------------------------------------------
// Blend factor calculation
// ---------------------------------------------------

float HistoryContrast(float historyLuma, float minNeighbourLuma, float maxNeighbourLuma)
{
    float lumaContrast = max(maxNeighbourLuma - minNeighbourLuma, 0) / historyLuma;
    float blendFactor = 1.0 / TAASampleCount;
    return saturate(blendFactor * rcp(1.0 + lumaContrast));
}

float GetBlendFactor(float colorLuma, float historyLuma, float minNeighbourLuma, float maxNeighbourLuma)
{
    return HistoryContrast(historyLuma, minNeighbourLuma, maxNeighbourLuma);
}

float GetLuminance(float3 color)
{
    return color.x;
}

float PerceptualWeight(float3 c)
{
    float luma = GetLuminance(c);
    return rcp(luma + 1.0);
}

float PerceptualInvWeight(float3 c)
{
    float luma = GetLuminance(c);
    return rcp(1.0 - luma);
}

float3 ConvertToWorkingSpace(float3 color)
{
    return RGBToYCoCg(color);
}

float3 ConvertToOutputSpace(float3 color)
{
    return YCoCgToRGB(color);
}

void LoadNeighbors(uint2 pixelIndex, int2 GTid, Texture2D image)
{
    GSBoxLoadStoreCoords coords = GetGSBoxLoadStoreCoords(pixelIndex, GTid, GlobalDataCB.PipelineRTResolution, GroupDimensionSize, SamplingRadius);

    float3 centerColor = ConvertToWorkingSpace(image[coords.LoadCoord0].rgb);
    centerColor *= PerceptualWeight(centerColor);

    gCache[coords.StoreCoord0.x][coords.StoreCoord0.y] = min16float3(centerColor);

    if (coords.IsLoadStore1Required)
    {
        float3 color = ConvertToWorkingSpace(image[coords.LoadCoord1].rgb);
        color *= PerceptualWeight(color);
        gCache[coords.StoreCoord1.x][coords.StoreCoord1.y] = min16float3(color);
    }

    if (coords.IsLoadStore2Required)
    {
        float3 color = ConvertToWorkingSpace(image[coords.LoadCoord2].rgb);
        color *= PerceptualWeight(color);
        gCache[coords.StoreCoord2.x][coords.StoreCoord2.y] = min16float3(color);
    }

    if (coords.IsLoadStore3Required)
    {
        float3 color = ConvertToWorkingSpace(image[coords.LoadCoord3].rgb);
        color *= PerceptualWeight(color);
        gCache[coords.StoreCoord3.x][coords.StoreCoord3.y] = min16float3(color);
    }

    GroupMemoryBarrierWithGroupSync();
}

float3 SampleHistory(Texture2D historyTex, float2 uv)
{
    float3 history = historyTex.SampleLevel(LinearClampSampler(), uv, 0.0).rgb;
    history = ConvertToWorkingSpace(history);
    history *= PerceptualWeight(history);
    return history;
}

AABB GetVarianceAABB(int2 GTindex, float3 center, float stDevMultiplier)
{
    float3 M1 = center; // First moment - Average
    float3 M2 = Square(center); // Second moment - Variance
    float sampleCount = 1.0;

    [unroll] 
    for (int x = -SamplingRadius; x <= SamplingRadius; ++x)
    {
        [unroll]
        for (int y = -SamplingRadius; y <= SamplingRadius; ++y)
        {
            if (x == 0 && y == 0)
                continue;

            int2 loadCoord = GTindex + int2(x, y);
            float3 color = gCache[loadCoord.x][loadCoord.y];

            M1 += color;
            M2 += Square(color);
            sampleCount += 1.0;
        }
    }

    float3 MU = M1 / sampleCount;
    float3 sigma = sqrt(max(M2 / sampleCount - MU * MU, 0.0));

    return InitAABB(MU - stDevMultiplier * sigma, MU + stDevMultiplier * sigma);
}

[numthreads(GroupDimensionSize, GroupDimensionSize, 1)]
void CSMain(int3 GTid : SV_GroupThreadID, int3 Gid : SV_GroupID)
{
    Texture2D previousFrameTexture = Textures2D[PassDataCB.PreviousFrameTexIdx];
    Texture2D currentFrameTexture = Textures2D[PassDataCB.CurrentFrameTexIdx];
    Texture2D<uint4> motionTexture = UInt4_Textures2D[PassDataCB.MotionTexIdx];
    RWTexture2D<float4> outputTexture = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    uint2 pixelIndex = ThreadGroupTilingX(PassDataCB.DispatchGroupCount, GroupDimensionSize.xx, 8, GTid.xy, Gid.xy);
    float3 motion = LoadGBufferMotion(motionTexture, pixelIndex);
    float2 uv = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);
    float2 reprojectedUV = uv - motion.xy;

    if (!FrameDataCB.IsTAAEnabled)
    {
        outputTexture[pixelIndex].rgb = currentFrameTexture[pixelIndex].rgb;
        return;
    }

    LoadNeighbors(pixelIndex, GTid.xy, currentFrameTexture);

    // Variance clipping
    // https://community.arm.com/developer/tools-software/graphics/b/blog/posts/temporal-anti-aliasing
    // https://developer.download.nvidia.com/gameworks/events/GDC2016/msalvi_temporal_supersampling.pdf
    // https://en.wikipedia.org/wiki/Moment_(mathematics)

    int2 groupThreadIndex = GTid.xy + SamplingRadius;
    float3 center = gCache[groupThreadIndex.x][groupThreadIndex.y];
    float3 history = SampleHistory(previousFrameTexture, reprojectedUV);
    float colorLuma = GetLuminance(center);
    float historyLuma = GetLuminance(history);

    float stDevMultiplier = 1.0;

    // The reasoning behind the anti flicker is that if we have high spatial contrast (high standard deviation)
    // and high temporal contrast, we let the history to be closer to be unclipped. To achieve, the min/max bounds
    // are extended artificially more.
    float temporalContrast = saturate(abs(colorLuma - historyLuma) / max(0.2, max(colorLuma, historyLuma)));
     
    float motionVectorLen = length(motion.xy);
    float screenDiag = length(float2(GlobalDataCB.PipelineRTResolution));
    const float MaxFactorScale = 2.25f; // when stationary
    const float MinFactorScale = 0.8f; // when moving more than slightly
    // Anti-flicker factor is shrunk under motion
    float localizedAntiFlicker = lerp(MinFactorScale, MaxFactorScale, saturate(1.0f - 2.0f * (motionVectorLen * screenDiag)));

    // Extend AABB if temporal contrast is high
    stDevMultiplier += lerp(0.0, localizedAntiFlicker, smoothstep(0.05, 0.55, temporalContrast));

    AABB aabb = GetVarianceAABB(groupThreadIndex, center, stDevMultiplier); 

    // Clip history to AABB
    float historyBlend = DistToAABB(center, history, aabb.Min, aabb.Max);
    float3 clippedHistory = lerp(history, center, historyBlend);

    // Compute blend factor for history
    float blendFactor = GetBlendFactor(colorLuma, historyLuma, aabb.Min.x, aabb.Max.x);
    blendFactor = 0.1;// max(0.03, blendFactor);

    // Blend history and current color
    float3 finalColor = lerp(clippedHistory, center, blendFactor);
    finalColor *= PerceptualInvWeight(finalColor);

    outputTexture[pixelIndex].rgb = ConvertToOutputSpace(finalColor);
}

#endif