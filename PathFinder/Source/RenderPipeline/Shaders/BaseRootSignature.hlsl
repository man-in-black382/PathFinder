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

struct Dummy { int D; };

#ifndef PassDataType
#define PassDataType Dummy
#endif

ConstantBuffer<GlobalData>      GlobalDataCB    : register(b0, space10);
ConstantBuffer<FrameData>       FrameDataCB     : register(b1, space10);
ConstantBuffer<PassDataType>    PassDataCB      : register(b2, space10);

Texture2D       Textures2D[]        : register(t0, space10);
Texture3D       Textures3D[]        : register(t0, space11);
Texture2DArray  Texture2DArrays[]   : register(t0, space12);

#ifndef RWTexture2DType
#define RWTexture2DType float4
#endif

#ifndef RWTexture2DArrayType
#define RWTexture2DArrayType float4
#endif

#ifndef RWTexture3DType
#define RWTexture3DType float4
#endif

RWTexture2D<RWTexture2DType>            RWTextures2D[]        : register(u0, space10);
RWTexture3D<RWTexture3DType>            RWTextures3D[]        : register(u0, space11);
RWTexture2DArray<RWTexture2DArrayType>  RWTexture2DArrays[]   : register(u0, space12);

