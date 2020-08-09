#ifndef _UIRenderPass__
#define _UIRenderPass__

struct PassData
{
    float4x4 ProjectionMatrix;
    uint UITextureSRVIndex;
};

#define PassDataType PassData

#include "MandatoryEntryPointInclude.hlsl"
#include "Packing.hlsl"

struct ImGuiVertex
{
    float2 Position;
    float2 UV;
    uint Color; // ImGui packs 4-comp colors in U32
};

struct ImGuiIndex
{
    uint Index;
};

struct Offsets
{
    uint VertexBufferOffset;
    uint IndexBufferOffset;
};

// Must be set as root constants to version offsets between draw calls
ConstantBuffer<Offsets> OffsetsCBuffer : register(b0);

StructuredBuffer<ImGuiVertex> UIVertexBuffer : register(t0);
StructuredBuffer<ImGuiIndex> UIIndexBuffer : register(t1);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv : TEXCOORD0;
};

PS_INPUT VSMain(uint indexId : SV_VertexID)
{
    uint index = UIIndexBuffer[indexId + OffsetsCBuffer.IndexBufferOffset].Index;
    ImGuiVertex vertex = UIVertexBuffer[index + OffsetsCBuffer.VertexBufferOffset];

    PS_INPUT output;
    output.pos = mul(PassDataCB.ProjectionMatrix, float4(vertex.Position, 0.f, 1.f));
    output.col = Decode8888(vertex.Color).abgr;
    output.uv = vertex.UV;
    return output;
};

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    Texture2D uiTexture = Textures2D[PassDataCB.UITextureSRVIndex];
    return uiTexture.Sample(LinearClampSampler(), input.uv).aaaa * input.col;
};

#endif