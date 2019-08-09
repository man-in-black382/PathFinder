struct VertexIn
{
    float3 Position : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;  
};

struct Camera
{
    float4x4 ViewProjection;
};

ConstantBuffer<Camera> CameraCB : register(b0, space0);
StructuredBuffer<int> SB : register(t0, space0);

VertexOut VSMain(VertexIn vin)
{
    SB[5] = 10;

    VertexOut vout;
    vout.Color = float4(0.6, 0.8, 1.0, 1.0);
    vout.PosH = mul(CameraCB.ViewProjection, float4(vin.Position, 1.0));
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    return pin.Color;
}