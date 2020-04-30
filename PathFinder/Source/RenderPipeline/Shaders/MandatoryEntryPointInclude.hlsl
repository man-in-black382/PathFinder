#ifndef _MandatoryEntryPointInclude__
#define _MandatoryEntryPointInclude__

#include "Camera.hlsl"

struct GlobalData
{
    uint2 PipelineRTResolution;
};

struct FrameData
{
    Camera Camera;
}; 

#define GlobalDataType GlobalData
#define FrameDataType FrameData

#include "BaseEngineLayout.hlsl"

#endif