#ifndef _MandatoryEntryPointInclude__
#define _MandatoryEntryPointInclude__

#include "Camera.hlsl"

struct GlobalData
{
    uint2 PipelineRTResolution;
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