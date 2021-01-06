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
    return abs(dot(orientation, UpY)) < 0.999 ? UpY : UpZ; 
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

float4x4 RotationMatrix4x4(float3 forward, float3 up)
{
    float3 zaxis = forward;
    float3 xaxis = normalize(cross(up, zaxis));
    float3 yaxis = cross(zaxis, xaxis);

    return Matrix4x4ColumnMajor(
        float4(xaxis, 0.0),
        float4(yaxis, 0.0),
        float4(zaxis, 0.0),
        float4(0.0, 0.0, 0.0, 1.0)
    );
}

float3x3 ReduceTo3x3(float4x4 m)
{
    float3x3 m3x3;
    m3x3[0] = m[0].xyz;
    m3x3[1] = m[1].xyz;
    m3x3[2] = m[2].xyz;
    return m3x3;
}

float3x3 RotationMatrix3x3(float3 forward)
{
    float3 up = GetUpVectorForOrientaion(forward);

    float3 zaxis = forward;
    float3 xaxis = normalize(cross(up, zaxis));
    float3 yaxis = cross(zaxis, xaxis);

    return Matrix3x3ColumnMajor(xaxis, yaxis, zaxis);
}

float3x3 Inverse(float3x3 m)
{
    float a00 = m[0][0], a01 = m[0][1], a02 = m[0][2];
    float a10 = m[1][0], a11 = m[1][1], a12 = m[1][2];
    float a20 = m[2][0], a21 = m[2][1], a22 = m[2][2];

    float b01 = a22 * a11 - a12 * a21;
    float b11 = -a22 * a10 + a12 * a20;
    float b21 = a21 * a10 - a11 * a20;

    float det = a00 * b01 + a01 * b11 + a02 * b21;

    return float3x3(b01, (-a22 * a01 + a02 * a21), (a12 * a01 - a02 * a11),
        b11, (a22 * a00 - a02 * a20), (-a12 * a00 + a02 * a10),
        b21, (-a21 * a00 + a01 * a20), (a11 * a00 - a01 * a10)) / det;
}

#endif