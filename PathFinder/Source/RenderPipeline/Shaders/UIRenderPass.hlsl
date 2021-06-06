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
#include "ColorConversion.hlsl"

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
    int TextureIdx;
    int SamplerIdx;
};

// Must be set as root constants to version offsets between draw calls
ConstantBuffer<Offsets> RootConstants : register(b0);

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
    uint index = UIIndexBuffer[indexId + RootConstants.IndexBufferOffset].Index;
    ImGuiVertex vertex = UIVertexBuffer[index + RootConstants.VertexBufferOffset];

    PS_INPUT output;
    output.pos = mul(PassDataCB.ProjectionMatrix, float4(vertex.Position, 0.f, 1.f));
    output.col = Decode8888(vertex.Color).abgr;
    output.uv = vertex.UV;
    return output;
};

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    float4 color = 0.0;

    if (RootConstants.TextureIdx >= 0)
    {
        Texture2D uiTexture = Textures2D[RootConstants.TextureIdx];
        color = uiTexture.Sample(Samplers[RootConstants.SamplerIdx], input.uv);
        color.a = 1.0;
    }
    else
    {
        Texture2D uiTexture = Textures2D[PassDataCB.UITextureSRVIndex];
        color = uiTexture.Sample(AnisotropicClampSampler(), input.uv).aaaa * input.col;
    }

    return color;
};

#endif