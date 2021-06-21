#ifndef _TAACommon__
#define _TAACommon__

#include "GroupsharedMemoryHelpers.hlsl"
#include "ColorConversion.hlsl"

static const int GroupDimensionSize = 16;
static const int SamplingRadius = 1;
static const int GSArrayDimensionSize = GroupDimensionSize + 2 * SamplingRadius;
static const float TAASampleCount = 16.0;

// ---------------------------------------------------
// History clipping
// ---------------------------------------------------

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

float3 ClipToAABB(float3 color, float3 history, float3 minimum, float3 maximum)
{
    float historyBlend = DistToAABB(color, history, minimum, maximum);
    float3 clippedHistory = lerp(history, color, historyBlend);
    return clippedHistory;
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

float GetLuminance(float3 ycocg)
{
    //return CIELuminance(ycocg);
    return ycocg.x;
}

float PerceptualWeight(float3 ycocg)
{
    float luma = GetLuminance(ycocg); 
    return rcp(luma + 1.0);
}

float PerceptualInvWeight(float3 ycocg)
{
    float luma = GetLuminance(ycocg);
    return rcp(1.0 - luma);
}

float3 ConvertToWorkingSpace(float3 rgb)
{
    float3 ycocg = RGBToYCoCg(rgb);
    //ycocg *= PerceptualWeight(ycocg);
    return ycocg;
}

float3 ConvertToOutputSpace(float3 ycocg)
{
    //ycocg *= PerceptualInvWeight(ycocg);
    float3 rgb = YCoCgToRGB(ycocg);
    return rgb;
}

#endif