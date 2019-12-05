float3x3 Matrix3x3ColumnMajor(float3 column0, float3 column1, float3 column2)
{
    float3x3 M;
    M[0] = float3(column0.x, column1.x, column2.x);
    M[1] = float3(column0.y, column1.y, column2.y);
    M[2] = float3(column0.z, column1.z, column2.z);
    return M;
}

float4x4 Matrix4x4ColumnMajor(float4 column0, float4 column1, float4 column2, float4 column3)
{
    float4x4 M;
    M[0] = float4(column0.x, column1.x, column2.x, column3.x);
    M[1] = float4(column0.y, column1.y, column2.y, column3.y);
    M[2] = float4(column0.z, column1.z, column2.z, column3.z);
    M[3] = float4(column0.w, column1.w, column2.w, column3.w);
    return M;
}

float Flatten3DIndexFloat(float3 index3D, float3 dimensions)
{
    return (index3D.x) + (index3D.y * dimensions.x) + (index3D.z * dimensions.x * dimensions.y);
}

int Flatten3DIndexInt(int3 index3D, int3 dimensions)
{
    return (index3D.x) + (index3D.y * dimensions.x) + (index3D.z * dimensions.x * dimensions.y);
}