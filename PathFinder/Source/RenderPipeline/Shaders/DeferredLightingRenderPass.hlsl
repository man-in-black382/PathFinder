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
    uint LTC_LUT0_Index;
    uint LTC_LUT1_Index;
    uint LTC_LUT_Size;
};

#define PassDataType PassData

struct RootConstants
{
    uint LightInstanceTableIndex;
};

#include "MandatoryEntryPointInclude.hlsl"
#include "GBuffer.hlsl"
#include "ColorConversion.hlsl"
#include "CookTorrance.hlsl"
#include "SpaceConversion.hlsl"
#include "Utils.hlsl"
#include "Matrix.hlsl"
#include "InstanceData.hlsl"

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);
StructuredBuffer<LightInstanceData> LightInstanceTable : register(t0);

static const float pi = 3.14159265;

// An extended version of the implementation from
// "How to solve a cubic equation, revisited"
// http://momentsingraphics.de/?p=105
float3 SolveCubic(float4 Coefficient)
{
    // Normalize the polynomial
    Coefficient.xyz /= Coefficient.w;
    // Divide middle coefficients by three
    Coefficient.yz /= 3.0;

    float A = Coefficient.w;
    float B = Coefficient.z;
    float C = Coefficient.y;
    float D = Coefficient.x;

    // Compute the Hessian and the discriminant
    float3 Delta = float3(
        -Coefficient.z * Coefficient.z + Coefficient.y,
        -Coefficient.y * Coefficient.z + Coefficient.x,
        dot(float2(Coefficient.z, -Coefficient.y), Coefficient.xy)
    );

    float Discriminant = dot(float2(4.0 * Delta.x, -Delta.y), Delta.zy);

    float3 RootsA, RootsD;

    float2 xlc, xsc;

    // Algorithm A
    {
        float A_a = 1.0;
        float C_a = Delta.x;
        float D_a = -2.0 * B * Delta.x + Delta.y;

        // Take the cubic root of a normalized complex number
        float Theta = atan2(sqrt(Discriminant), -D_a) / 3.0;

        float x_1a = 2.0 * sqrt(-C_a) * cos(Theta);
        float x_3a = 2.0 * sqrt(-C_a) * cos(Theta + (2.0 / 3.0) * pi);

        float xl;
        if ((x_1a + x_3a) > 2.0 * B)
            xl = x_1a;
        else
            xl = x_3a;

        xlc = float2(xl - B, A);
    }

    // Algorithm D
    {
        float A_d = D;
        float C_d = Delta.z;
        float D_d = -D * Delta.y + 2.0 * C * Delta.z;

        // Take the cubic root of a normalized complex number
        float Theta = atan2(D * sqrt(Discriminant), -D_d) / 3.0;

        float x_1d = 2.0 * sqrt(-C_d) * cos(Theta);
        float x_3d = 2.0 * sqrt(-C_d) * cos(Theta + (2.0 / 3.0) * pi);

        float xs;
        if (x_1d + x_3d < 2.0 * C)
            xs = x_1d;
        else
            xs = x_3d;

        xsc = float2(-D, xs + C);
    }

    float E = xlc.y * xsc.y;
    float F = -xlc.x * xsc.y - xlc.y * xsc.x;
    float G = xlc.x * xsc.x;

    float2 xmc = float2(C * F - B * G, -B * F + C * E);

    float3 Root = float3(xsc.x / xsc.y, xmc.x / xmc.y, xlc.x / xlc.y);

    if (Root.x < Root.y && Root.x < Root.z)
        Root.xyz = Root.yxz;
    else if (Root.z < Root.x && Root.z < Root.y)
        Root.xyz = Root.xzy;

    return Root;
}

// Linearly Transformed Cosines
///////////////////////////////

float3 LTC_Evaluate(
    float3 N, float3 V, float3 P, float3x3 Minv, float3 points[4], Texture2D LTC_LUT1, float2 LUTSize)
{
    // construct orthonormal basis around N
    float3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    float3x3 R = transpose(Matrix3x3ColumnMajor(T1, T2, N));

    // polygon (allocate 5 vertices for clipping)
    float3 L_[3];
    L_[0] = mul(R, points[0] - P);
    L_[1] = mul(R, points[1] - P);
    L_[2] = mul(R, points[2] - P);

    float3 Lo_i = float3(0.0, 0.0, 0.0);

    // init ellipse
    float3 C = 0.5 * (L_[0] + L_[2]);
    float3 V1 = 0.5 * (L_[1] - L_[2]);
    float3 V2 = 0.5 * (L_[1] - L_[0]);

    C = mul(Minv, C);
    V1 = mul(Minv, V1);
    V2 = mul(Minv, V2);

    // compute eigenvectors of ellipse
    float a, b;
    float d11 = dot(V1, V1);
    float d22 = dot(V2, V2);
    float d12 = dot(V1, V2);
    if (abs(d12) / sqrt(d11 * d22) > 0.0001)
    {
        float tr = d11 + d22;
        float det = -d12 * d12 + d11 * d22;

        // use sqrt matrix to solve for eigenvalues
        det = sqrt(det);
        float u = 0.5 * sqrt(tr - 2.0 * det);
        float v = 0.5 * sqrt(tr + 2.0 * det);
        float e_max = Square(u + v);
        float e_min = Square(u - v);

        float3 V1_, V2_;

        if (d11 > d22)
        {
            V1_ = d12 * V1 + (e_max - d11) * V2;
            V2_ = d12 * V1 + (e_min - d11) * V2;
        }
        else
        {
            V1_ = d12 * V2 + (e_max - d22) * V1;
            V2_ = d12 * V2 + (e_min - d22) * V1;
        }

        a = 1.0 / e_max;
        b = 1.0 / e_min;
        V1 = normalize(V1_);
        V2 = normalize(V2_);
    }
    else
    {
        a = 1.0 / dot(V1, V1);
        b = 1.0 / dot(V2, V2);
        V1 *= sqrt(a);
        V2 *= sqrt(b);
    }

    float3 V3 = cross(V1, V2);
    if (dot(C, V3) < 0.0)
        V3 *= -1.0;

    float L = dot(V3, C);
    float x0 = dot(V1, C) / L;
    float y0 = dot(V2, C) / L;

    float E1 = rsqrt(a);
    float E2 = rsqrt(b);

    a *= L * L;
    b *= L * L;

    float c0 = a * b;
    float c1 = a * b * (1.0 + x0 * x0 + y0 * y0) - a - b;
    float c2 = 1.0 - a * (1.0 + x0 * x0) - b * (1.0 + y0 * y0);
    float c3 = 1.0;

    float3 roots = SolveCubic(float4(c0, c1, c2, c3));
    float e1 = roots.x;
    float e2 = roots.y;
    float e3 = roots.z;

    float3 avgDir = float3(a * x0 / (a - e2), b * y0 / (b - e2), 1.0);

    float3x3 rotate = Matrix3x3ColumnMajor(V1, V2, V3);

    avgDir = mul(rotate, avgDir);
    avgDir = normalize(avgDir);

    float L1 = sqrt(-e2 / e3);
    float L2 = sqrt(-e2 / e1);

    float formFactor = L1 * L2 * rsqrt((1.0 + L1 * L1) * (1.0 + L2 * L2));

    // use tabulated horizon-clipped sphere
    float2 uv = float2(avgDir.z * 0.5 + 0.5, formFactor);
    uv = Refit0to1ValuesToTexelCenter(uv, LUTSize);

    float scale = LTC_LUT1.SampleLevel(LinearClampSampler, uv, 0).w;
    float spec = formFactor * scale;

    Lo_i = float3(spec, spec, spec);
    
    return float3(Lo_i);
}

DiskLightPoints GetDiskLightPoints(LightInstanceData light)
{
    DiskLightPoints points;

    float halfWidth = light.Width * 0.5;
    float halfHeight = light.Height * 0.5;

    float3 position = light.Position.xyz;
    float3 orientation = light.Orientation.xyz;

    // Get billboard points at the origin
    float4 p0 = float4(-halfWidth, -halfHeight, 0.0, 0.0);
    float4 p1 = float4(halfWidth, -halfHeight, 0.0, 0.0);
    float4 p2 = float4(halfWidth, halfHeight, 0.0, 0.0);
    float4 p3 = float4(-halfWidth, halfHeight, 0.0, 0.0);

    float4x4 diskRotation = LookAtMatrix(orientation, GetUpVectorForOrientaion(orientation));

    // Rotate around origin
    p0 = mul(diskRotation, p0);
    p1 = mul(diskRotation, p1);
    p2 = mul(diskRotation, p2);
    p3 = mul(diskRotation, p3);

    // Move points to disk light's location
    // Order of points is picked to match LTC convention
    points.Points[1] = p0.xyz + light.Position.xyz;
    points.Points[0] = p1.xyz + light.Position.xyz;
    points.Points[2] = p2.xyz + light.Position.xyz;
    points.Points[3] = p3.xyz + light.Position.xyz;

    return points;
}

float3 EvaluateDiskLight(LightInstanceData light, float3 V, float3 surfaceWPos, GBuffer gBuffer, Texture2D LTC_LUT0, Texture2D LTC_LUT1, float2 LUTSize)
{
    DiskLightPoints diskPoints = GetDiskLightPoints(light);

    float NdotV = saturate(dot(gBuffer.Normal, V));
    float2 uv = float2(gBuffer.Roughness, sqrt(1.0 - NdotV));
    uv = Refit0to1ValuesToTexelCenter(uv, LUTSize);

    float4 t1 = LTC_LUT0.SampleLevel(LinearClampSampler, uv, 0);
    float4 t2 = LTC_LUT1.SampleLevel(LinearClampSampler, uv, 0);

    float3x3 Minv = Matrix3x3ColumnMajor(
        float3(t1.x, 0, t1.y),
        float3(0, 1, 0),
        float3(t1.z, 0, t1.w)
    );

    float geometryMasking = t2.x;
    float fresnel = t2.y;

    float3 specular = LTC_Evaluate(gBuffer.Normal, V, surfaceWPos, Minv, diskPoints.Points, LTC_LUT1, LUTSize);
    // BRDF shadowing and Fresnel
    float3 f90 = lerp(0.04, gBuffer.Albedo, gBuffer.Metalness);
    specular *= f90 * geometryMasking + (1.0 - f90) * fresnel;
    
    float3 diffuse = LTC_Evaluate(gBuffer.Normal, V, surfaceWPos, Matrix3x3Identity, diskPoints.Points, LTC_LUT1, LUTSize);
    diffuse *= gBuffer.Albedo * (1.0 - gBuffer.Metalness) * (1.0 - fresnel);

    return (specular + diffuse) * light.Color.rgb * light.LuminousIntensity;
}

float3 EvaluateDirectLighting(LightInstanceData light, float3 V, float3 surfaceWPos, GBuffer gBuffer, Texture2D LTC_LUT0, Texture2D LTC_LUT1)
{
    float3 integrationResult = 0.0.xxx;
    float2 LUTSize = float2(PassDataCB.LTC_LUT_Size, PassDataCB.LTC_LUT_Size);
    
    switch (light.LightType)
    {
        case LightTypeDisk:
            integrationResult = EvaluateDiskLight(light, V, surfaceWPos, gBuffer, LTC_LUT0, LTC_LUT1, LUTSize);
            break;
        default:
            integrationResult = float3(1.0, 0.0, 0.0);
            break;
    }
        
    return integrationResult;
}

[numthreads(32, 32, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID)
{   
    Texture2D LTC_LUT0 = Textures2D[PassDataCB.LTC_LUT0_Index];
    Texture2D LTC_LUT1 = Textures2D[PassDataCB.LTC_LUT1_Index];
    Texture2D<uint4> materialData = UInt4_Textures2D[PassDataCB.GBufferMaterialDataTextureIndex];
    Texture2D depthTexture = Textures2D[PassDataCB.GBufferDepthTextureIndex];
    LightInstanceData light = LightInstanceTable[RootConstantBuffer.LightInstanceTableIndex];

    uint2 pixelIndex = dispatchThreadID.xy;
    float2 UV = TexelIndexToUV(pixelIndex, GlobalDataCB.PipelineRTResolution);

    GBufferEncoded encodedGBuffer;
    encodedGBuffer.MaterialData = materialData.Load(uint3(pixelIndex, 0));
    
    GBuffer gBuffer = DecodeGBuffer(encodedGBuffer);

    float depth = depthTexture.Load(uint3(dispatchThreadID.xy, 0)).r;
    float3 worldPosition = ReconstructWorldPosition(depth, UV, FrameDataCB.CameraInverseView, FrameDataCB.CameraInverseProjection);
    float3 V = normalize(FrameDataCB.CameraPosition.xyz - worldPosition);

    float3 outgoingRadiance = EvaluateDirectLighting(light, V, worldPosition, gBuffer, LTC_LUT0, LTC_LUT1);
    
    float3 L = float3(0.0, 1.0, 0.0);
    float3 H = normalize(L + V);

    //float3 outgoingRadiance = BRDF(gBuffer.Normal, V, H, L, gBuffer.Roughness, gBuffer.Albedo, gBuffer.Metalness, 250.xxx);

    RWTexture2D<float4> outputImage = RW_Float4_Textures2D[PassDataCB.OutputTextureIndex];
    outputImage[dispatchThreadID.xy] = float4(outgoingRadiance, 1.0);
}