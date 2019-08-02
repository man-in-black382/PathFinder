struct VertexIn
{
    float3 Position : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;  
};

//cbuffer CB : register(b0)
//{
//    int a;
//};
//
//struct Test
//{
//
//};
//
//ConstantBuffer<Test> cb1 : register(b1);
//StructuredBuffer<Test> cb1 : register(b1);

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    vout.Color = float4(1.0, 1.0, 1.0, 1.0);
    vout.PosH = float4(vin.Position, 1.0);
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    return pin.Color;
}