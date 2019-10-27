struct DirectionalLight
{
    float3 RadiantFlux; // a.k.a color
    float3 Direction;
};

struct PassData
{
    uint GBufferMaterialDataTextureIndex;
};

#define PassDataType PassData

#include "BaseRootSignature.hlsl"
#include "GBuffer.hlsl"
#include "ColorConversion.hlsl"
#include "FullScreenQuadVS.hlsl"

//------------------------  Pixel  ------------------------------//

float4 PSMain(VertexOut pin) : SV_Target
{
    Texture2D materialData = Textures2D[PassDataCB.GBufferMaterialDataTextureIndex];
    
    GBufferEncoded encodedGBuffer;
    encodedGBuffer.MaterialData = materialData.Sample(PointClampSampler, pin.UV);

    GBufferCookTorrance gBufferCookTorrance = DecodeGBufferCookTorrance(encodedGBuffer);

    return float4(gBufferCookTorrance.Albedo, 1.0);
}