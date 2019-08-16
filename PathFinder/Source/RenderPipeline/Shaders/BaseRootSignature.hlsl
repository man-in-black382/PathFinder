struct GlobalData
{
    float2 PipelineRTResolution;
};

struct FrameData
{
    float4 CameraPosition;
    float4x4 CameraView;
    float4x4 CameraProjection;
    float4x4 CameraViewProjection;
    float4x4 CameraInverseView;
    float4x4 CameraInverseProjection;
    float4x4 CameraInverseViewProjection;
}; 

#ifndef PassDataType
#define PassDataType int
#endif

Texture2D       Textures2D[]        : register(t0, space0);
Texture3D       Textures3D[]        : register(t0, space1);
Texture2DArray  Texture2DArrays[]   : register(t0, space2);

ConstantBuffer<GlobalData>      GlobalDataCB    : register(b0, space0);
ConstantBuffer<FrameData>       FrameDataCB     : register(b1, space0);
ConstantBuffer<PassDataType>    PassDataCB      : register(b2, space0);

