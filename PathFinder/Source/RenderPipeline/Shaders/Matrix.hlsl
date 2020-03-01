#ifndef _Matrix__
#define _Matrix__

static const float3 UpY = float3(0.0, 1.0, 0.0);
static const float3 UpZ = float3(0.0, 0.0, 1.0);

static const float3x3 Matrix3x3Identity = 
{
    float3(1, 0, 0),
    float3(0, 1, 0),
    float3(0, 0, 1)
};

static const float4x4 Matrix4x4Identity =
{
    float4(1, 0, 0, 0),
    float4(0, 1, 0, 0),
    float4(0, 0, 1, 0),
    float4(0, 0, 0, 1)
};

float3 GetUpVectorForOrientaion(float3 orientation)
{
    return abs(dot(orientation, UpY)) < 0.9999 ? UpY : UpZ; 
}

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

float4x4 AxisMatrix(float3 right, float3 up, float3 forward)
{
    float3 xaxis = right;
    float3 yaxis = up;
    float3 zaxis = forward;

    return float4x4(
        xaxis.x, yaxis.x, zaxis.x, 0,
        xaxis.y, yaxis.y, zaxis.y, 0,
        xaxis.z, yaxis.z, zaxis.z, 0,
        0, 0, 0, 1
    );
}

float4x4 LookAtMatrix(float3 forward, float3 up)
{
    float3 zaxis = forward;
    float3 xaxis = normalize(cross(up, zaxis));
    float3 yaxis = cross(zaxis, xaxis);

    return AxisMatrix(xaxis, yaxis, zaxis);
}

float4x4 LookAtMatrix(float3 at, float3 eye, float3 up)
{
    float3 zaxis = normalize(at - eye);
    float3 xaxis = normalize(cross(up, zaxis));
    float3 yaxis = cross(zaxis, xaxis);

    return AxisMatrix(xaxis, yaxis, zaxis);
}

#endif