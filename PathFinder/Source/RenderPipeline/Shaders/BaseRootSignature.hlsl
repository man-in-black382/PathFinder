struct GlobalData
{
    uint2 PipelineRTResolution;
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

ConstantBuffer<GlobalData>      GlobalDataCB    : register(b0, space0);
ConstantBuffer<FrameData>       FrameDataCB     : register(b1, space0);
ConstantBuffer<PassDataType>    PassDataCB      : register(b2, space0);

Texture2D       Textures2D[]        : register(t0, space0);
Texture3D       Textures3D[]        : register(t0, space1);
Texture2DArray  Texture2DArrays[]   : register(t0, space2);

#ifndef RWTexture2DType
#define RWTexture2DType float4
#endif

#ifndef RWTexture2DArrayType
#define RWTexture2DArrayType float4
#endif

#ifndef RWTexture3DType
#define RWTexture3DType float4
#endif

RWTexture2D<RWTexture2DType>            RWTextures2D[]        : register(u0, space0);
RWTexture3D<RWTexture3DType>            RWTextures3D[]        : register(u0, space1);
RWTexture2DArray<RWTexture2DArrayType>  RWTexture2DArrays[]   : register(u0, space2);

