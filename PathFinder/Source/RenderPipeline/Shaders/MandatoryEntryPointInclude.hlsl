#ifndef _MandatoryEntryPointInclude__
#define _MandatoryEntryPointInclude__

#include "Camera.hlsl"

struct GlobalData
{
    float2 PipelineRTResolution;
    float2 PipelineRTResolutionInv;

    uint AnisotropicClampSamplerIdx;
    uint LinearClampSamplerIdx;
    uint PointClampSamplerIdx;
    uint MinSamplerIdx;
    uint MaxSamplerIdx;
};

struct FrameData
{
    Camera CurrentFrameCamera;
    Camera PreviousFrameCamera;

    bool IsDenoiserEnabled;
    bool IsReprojectionHistoryDebugEnabled;
    bool IsGradientDebugEnabled;
    bool IsMotionDebugEnabled;
    bool IsDenoiserAntilagEnabled;
}; 

#define GlobalDataType GlobalData
#define FrameDataType FrameData

#include "BaseEngineLayout.hlsl"

sampler AnisotropicClampSampler() { return Samplers[GlobalDataCB.AnisotropicClampSamplerIdx]; }
sampler LinearClampSampler() { return Samplers[GlobalDataCB.LinearClampSamplerIdx]; }
sampler PointClampSampler() { return Samplers[GlobalDataCB.PointClampSamplerIdx]; }
sampler MinSampler() { return Samplers[GlobalDataCB.MinSamplerIdx]; }
sampler MaxSampler() { return Samplers[GlobalDataCB.MaxSamplerIdx]; }

#endif