#ifndef _BaseEngineLayout__
#define _BaseEngineLayout__

// The maximum size of a root signature is 64 DWORDs :
// Descriptor tables cost 1 DWORD each.
// Root constants cost 1 DWORD each, since they are 32 - bit values.
// Root descriptors(64 - bit GPU virtual addresses) cost 2 DWORDs each.
// Static samplers do not have any cost in the size of the root signature.

struct Dummy { int D; };

#ifndef GlobalDataType
#define GlobalDataType Dummy
#endif

#ifndef FrameDataType
#define FrameDataType Dummy
#endif

#ifndef PassDataType
#define PassDataType Dummy
#endif

ConstantBuffer<GlobalDataType>  GlobalDataCB    : register(b0, space10);
ConstantBuffer<FrameDataType>   FrameDataCB     : register(b1, space10);
ConstantBuffer<PassDataType>    PassDataCB      : register(b2, space10);

// Discussion on texture/buffer casting to avoid occupying multiple registers 
// by unbounded texture arrays when different types (raw, uint, int, float etc.) are required.
// Solution will hopefuly arrive in HLSL 6.6 (2020?)
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
RWTexture2D<uint>        RW_UInt_Textures2D[]         : register(u0, space12);

RWTexture3D<float4>      RW_Float4_Textures3D[]       : register(u0, space13);
RWTexture3D<uint4>       RW_UInt4_Textures3D[]        : register(u0, space14);

RWTexture2DArray<float4> RW_Float4_Texture2DArrays[]  : register(u0, space15);

SamplerState Samplers[] : register(s0, space10);

// GPU Data Inspection
RWStructuredBuffer<float> DebugReadbackBuffer : register(u0, space16);

static uint DebugBufferWriteIndex = 0;
static uint DebugBufferWrittenObjectCount = 0;
static bool DebugBufferWriteCondition = false;

static const uint DebugReadbackTypeFloat = 0;
static const uint DebugReadbackTypeFloat2 = 1;
static const uint DebugReadbackTypeFloat3 = 2;
static const uint DebugReadbackTypeFloat4 = 3;

void SetDataInspectorWriteCondition(bool condition)
{
    DebugBufferWriteCondition = condition;
}

void OutputDataInspectorValue(float v)
{
    if (!DebugBufferWriteCondition)
        return;

    DebugReadbackBuffer[++DebugBufferWriteIndex] = DebugReadbackTypeFloat;
    DebugReadbackBuffer[++DebugBufferWriteIndex] = v;
    DebugReadbackBuffer[0] = ++DebugBufferWrittenObjectCount;
}

void OutputDataInspectorValue(float2 v)
{
    if (!DebugBufferWriteCondition)
        return;

    DebugReadbackBuffer[++DebugBufferWriteIndex] = DebugReadbackTypeFloat2;
    DebugReadbackBuffer[++DebugBufferWriteIndex] = v.x;
    DebugReadbackBuffer[++DebugBufferWriteIndex] = v.y;
    DebugReadbackBuffer[0] = ++DebugBufferWrittenObjectCount;
}

void OutputDataInspectorValue(float3 v)
{
    if (!DebugBufferWriteCondition)
        return;

    DebugReadbackBuffer[++DebugBufferWriteIndex] = DebugReadbackTypeFloat3;
    DebugReadbackBuffer[++DebugBufferWriteIndex] = v.x;
    DebugReadbackBuffer[++DebugBufferWriteIndex] = v.y;
    DebugReadbackBuffer[++DebugBufferWriteIndex] = v.z;
    DebugReadbackBuffer[0] = ++DebugBufferWrittenObjectCount;
}

void OutputDataInspectorValue(float4 v)
{
    if (!DebugBufferWriteCondition)
        return;

    DebugReadbackBuffer[++DebugBufferWriteIndex] = DebugReadbackTypeFloat4;
    DebugReadbackBuffer[++DebugBufferWriteIndex] = v.x;
    DebugReadbackBuffer[++DebugBufferWriteIndex] = v.y;
    DebugReadbackBuffer[++DebugBufferWriteIndex] = v.z;
    DebugReadbackBuffer[++DebugBufferWriteIndex] = v.w;
    DebugReadbackBuffer[0] = ++DebugBufferWrittenObjectCount;
}

#endif