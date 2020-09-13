#ifndef _SMAACommon__
#define _SMAACommon__

#include "MandatoryEntryPointInclude.hlsl"

#define SMAA_RT_METRICS float4(GlobalDataCB.PipelineRTResolutionInv, GlobalDataCB.PipelineRTResolution)
#define SMAA_HLSL_4_1
#define SMAA_PRESET_ULTRA

#define SMAA_LINEAR_CLAMP_SAMPLER LinearClampSampler()
#define SMAA_POINT_CLAMP_SAMPLER PointClampSampler()

// https://rauwendaal.net/2014/06/14/rendering-a-screen-covering-triangle-in-opengl/
// 
static const float2 Vertices[3] = { float2(-1.0, -1.0), float2(-1.0, 3.0), float2(3.0, -1.0) };
static const float2 UVs[3] = { float2(0.0, 0.0), float2(0.0, 2.0), float2(2.0, 0.0) };

#endif