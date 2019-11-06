struct DirectionalLight
{
    float3 RadiantFlux; // a.k.a color
    float3 Direction;
};

struct PassData
{
    uint GBufferMaterialDataTextureIndex;
    uint GBufferDepthTextureIndex;
    uint OutputTextureIndex;
};

#define PassDataType PassData

#include "BaseRootSignature.hlsl"
#include "GBuffer.hlsl"
#include "ColorConversion.hlsl"
#include "CookTorrance.hlsl"
#include "SpaceConversion.hlsl"

//------------------------  Pixel  ------------------------------//

[numthreads(32, 32, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    Texture2D<uint4> materialData = UInt4_Textures2D[PassDataCB.GBufferMaterialDataTextureIndex];
    Texture2D depthTexture = Textures2D[PassDataCB.GBufferDepthTextureIndex];
    
    uint3 loadCoords = uint3(dispatchThreadID.xy, 0);
    float2 UV = (float2(dispatchThreadID.xy) + 0.5) / GlobalDataCB.PipelineRTResolution;

    GBufferEncoded encodedGBuffer;
    encodedGBuffer.MaterialData = materialData.Load(loadCoords);

    GBufferCookTorrance gBufferCookTorrance = DecodeGBufferCookTorrance(encodedGBuffer);
    DirectionalLight testLight = { float3(2.0, 2.0, 2.0), float3(-1.0, -1.0, -1.0) };
    float depth = depthTexture.Load(uint3(dispatchThreadID.xy, 0));

    float3 worldPosition = ReconstructWorldPosition(depth, UV, FrameDataCB.CameraInverseView, FrameDataCB.CameraInverseProjection);

    // Based on observations by Disney and adopted by Epic Games
    // the lighting looks more correct squaring the roughness
    // in both the geometry and normal distribution function.
    float roughness2 = gBufferCookTorrance.Roughness * gBufferCookTorrance.Roughness;

    float3 N = gBufferCookTorrance.Normal;
    float3 V = normalize(FrameDataCB.CameraPosition - worldPosition);
    float3 L = -normalize(testLight.Direction);
    float3 H = normalize(L + V);

    float3 radiance = CookTorranceBRDF(N, V, H, L, roughness2, gBufferCookTorrance.Albedo, gBufferCookTorrance.Metalness, testLight.RadiantFlux);

    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];
    outputImage[dispatchThreadID.xy] = float4(N, 1.0);
}