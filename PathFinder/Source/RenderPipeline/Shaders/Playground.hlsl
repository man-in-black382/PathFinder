struct VertexIn
{
    float4 PosL : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;  
};

VertexOut VSMain(VertexIn vin, uint vertexId : SV_VertexID)
{
  /*  const float4 Verts[3] = {
        float4(0.0, 0.0, 0.0, 1.0),
        float4(0.0, 0.0, 0.0, 1.0), 
        float4(0.0, 0.0, 0.0, 1.0)
    };*/

    VertexOut vout;
    vout.Color = vin.Color;
    vout.PosH = vin.PosL;
    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    return pin.Color;
}
