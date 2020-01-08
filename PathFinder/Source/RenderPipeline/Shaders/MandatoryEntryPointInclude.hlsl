// The maximum size of a root signature is 64 DWORDs :
// Descriptor tables cost 1 DWORD each.
// Root constants cost 1 DWORD each, since they are 32 - bit values.
// Root descriptors(64 - bit GPU virtual addresses) cost 2 DWORDs each.
// Static samplers do not have any cost in the size of the root signature.

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

// Discussion on texture/buffer casting to avoid occupying multiple registers 
// by unbounded texture arrays when different types (raw, uint, int, float etc.) are required
// https://github.com/microsoft/DirectXShaderCompiler/issues/1067
//
// For now we have to declare and bind each type separately

// SRV Raw and Typed boundless descriptor ranges
Texture2D        Textures2D[]       : register(t0, space10);
Texture2D<uint4> UInt4_Textures2D[] : register(t0, space11);

Texture3D        Textures3D[]       : register(t0, space12);
Texture3D<uint4> UInt4_Textures3D[] : register(t0, space13);

Texture2DArray   Texture2DArrays[]  : register(t0, space14);

// UAV Raw and Typed boundless descriptor ranges
RWTexture2D<float4>      RW_Float4_Textures2D[]       : register(u0, space10);
RWTexture2D<uint4>       RW_UInt4_Textures2D[]        : register(u0, space11);

RWTexture3D<float4>      RW_Float4_Textures3D[]       : register(u0, space12);
RWTexture3D<uint4>       RW_UInt4_Textures3D[]        : register(u0, space13);

RWTexture2DArray<float4> RW_Float4_Texture2DArrays[]  : register(u0, space14);

// Static samplers
SamplerState AnisotropicClampSampler : register(s0);
SamplerState LinearClampSampler : register(s1);
SamplerState PointClampSampler : register(s2);

// Debug

struct ReadBackData
{
    float Value;
};

RWStructuredBuffer<ReadBackData> DebugReadbackBuffer : register(u0, space15);

static uint DebugBufferWriteIndex = 1;

void DebugOut(float value, uint2 currentPixel, uint2 targetPixel)
{
    if (!all(currentPixel == targetPixel)) return;

    DebugReadbackBuffer[DebugBufferWriteIndex].Value = value;
    DebugReadbackBuffer[0].Value = DebugBufferWriteIndex;
    DebugBufferWriteIndex += 1;
}

