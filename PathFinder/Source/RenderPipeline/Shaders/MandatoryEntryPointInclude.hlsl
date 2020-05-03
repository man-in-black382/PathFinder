#ifndef _MandatoryEntryPointInclude__
#define _MandatoryEntryPointInclude__

#include "Camera.hlsl"

struct GlobalData
{
    float2 PipelineRTResolution;
    float2 PipelineRTResolutionInv;
};

struct FrameData
{
    Camera CurrentFrameCamera;
    Camera PreviousFrameCamera;
}; 

#define GlobalDataType GlobalData
#define FrameDataType FrameData

#include "BaseEngineLayout.hlsl"

#endif