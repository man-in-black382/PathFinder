//struct VertexIn
//{
//    float3 PosL : POSITION;
//    float4 Color : COLOR;
//};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;  
};

//#ifdef VSEntryPoint
VertexOut VSMain( /*VertexIn vin*/int vertexId : SV_VertexID)
{
    const float4 Verts[3] = { float4(0.0, 0.0, 0.0, 1.0), float4(0.0, 0.0, 0.0, 1.0), float4(0.0, 0.0, 0.0, 1.0) };

    VertexOut vout;
    vout.Color = float4(1.0, 0.4, 0.7, 1.0);
    vout.PosH = Verts[vertexId];
    return vout;
}
//#endif
//
//#ifdef PSEntryPoint
//float4 PSMain(VertexOut pin) : SV_Target
//{
//    return pin.Color;
//}
//#endif