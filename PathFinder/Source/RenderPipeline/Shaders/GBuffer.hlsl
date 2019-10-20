struct InstanceData
{
    float4x4 ModelMatrix;
    uint AlbedoMapIndex;
    uint NormalMapIndex;
    uint RoughnessMapIndex;
    uint MetalnessMapIndex;
    uint AOMapIndex;
};

struct DirectionalLight
{
    float3 RadiantFlux; // a.k.a color
    float3 Direction;
};

#include "BaseRootSignature.hlsl"
#include "CookTorrance.hlsl"

ConstantBuffer<InstanceData> InstanceTable : register(b0);

struct VertexIn
{
    float4 Position : POSITION0;
    float3 Normal : NORMAL0;
    float2 UV : TEXCOORD0;
};

struct VertexOut
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;  
};

float3 LinearFromSRGB(float3 sRGB)
{
    return pow(sRGB, 2.2);
}

float3 SRGBFromLinear(float3 linearColor)
{
    return pow(linearColor, 1.0 / 2.2);
}

//float3x3 TBN()
//{
//    vec3 T = normalize(uNormalMat * vec4(iTangent, 0.0)).xyz;
//    vec3 B = normalize(uNormalMat * vec4(iBitangent, 0.0)).xyz;
//    vec3 N = normalize(uNormalMat * vec4(iNormal, 0.0)).xyz;
//    return mat3(T, B, N);
//}

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    
    float4x4 MVP = mul(FrameDataCB.CameraViewProjection, InstanceTable.ModelMatrix);

    vout.Position = mul(MVP, vin.Position);
    vout.UV = vin.UV;

    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    Texture2D albedoMap = Textures2D[InstanceTable.AlbedoMapIndex];
    Texture2D normalMap = Textures2D[InstanceTable.NormalMapIndex];
    Texture2D roughnessMap = Textures2D[InstanceTable.RoughnessMapIndex];
    Texture2D metalnessMap = Textures2D[InstanceTable.MetalnessMapIndex];

    return albedoMap.Load(int3(pin.UV * float2(4096, 4096), 0));
}