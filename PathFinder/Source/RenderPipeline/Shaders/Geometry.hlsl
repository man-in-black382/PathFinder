#ifndef _Geometry__
#define _Geometry__

#include "Utils.hlsl"

struct Ray
{
    float3 Origin;
    float3 Direction;
    float TMin;
    float TMax;
};

struct Sphere
{
    float3 Center;
    float Radius;
};

// q01 ----------- q10
// |                 |
// |                 |  
// |                 |  
// q00 ----------- q11
struct BilinearPatch
{
    float3 Q00;
    float3 Q01;
    float3 Q10;
    float3 Q11;
};

struct Plane
{
    float3 Normal;
    float3 PointOnPlane;
};

Sphere InitSphere(float3 center, float radius)
{
    Sphere sphere;
    sphere.Center = center;
    sphere.Radius = radius;
    return sphere;
}

Ray InitRay(float3 origin, float3 direction, float tmin, float tmax)
{
    Ray ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = tmin;
    ray.TMax = tmax;
    return ray;
}

Ray InitRay(float3 origin, float3 direction)
{
    return InitRay(origin, direction, 0.0, FloatMax);
}

Plane InitPlane(float3 normal, float3 pointOnPlane)
{
    Plane plane;
    plane.Normal = normal;
    plane.PointOnPlane = pointOnPlane;
    return plane;
}

Plane InitPlane(float3 normal, float displacement)
{
    return InitPlane(normal, displacement * normal);
}

float3 InterpolatePatch(BilinearPatch patch, float2 uv)
{
    return (1.0 - uv.x) * (1.0 - uv.y) * patch.Q00 +
        (1.0 - uv.x) * uv.y * patch.Q01 +
        uv.x * (1.0 - uv.y) * patch.Q10 +
        uv.x * uv.y * patch.Q11;
}

// Ray Tracing Gems: A Geometric Approach to Ray/Bilinear Patch Intersections
//
bool RayBilinearPatchIntersection(BilinearPatch patch, Ray ray, out float3 intersectionPoint)
{
    // 4 corners + "normal" qn 
    float3 q00 = patch.Q00;
    float3 q10 = patch.Q11; // Swap to match Nvidia convention
    float3 q11 = patch.Q10; // Swap to match Nvidia convention
    float3 q01 = patch.Q01;

    float3 e10 = q10 - q00; // q01 ----------- q11
    float3 e11 = q11 - q10; // |                 |
    float3 e00 = q01 - q00; // | e00         e11 |  we precompute 
                            // |       e10       |  qn = cross(q10-q00, 
                            // q00 ----------- q10             q01-q11) 

    float3 qn = cross(q10 - q00, q01 - q11); 

    q00 -= ray.Origin;
    q10 -= ray.Origin;

    float a = dot(cross(q00, ray.Direction), e00);  // the equation is 
    float c = dot(qn, ray.Direction);               // a + b u + c u^2 
    float b = dot(cross(q10, ray.Direction), e11);  // first compute

    b -= a + c;                                     // a+b+c and then b 
    float det = b * b - 4 * a * c;
    if (det < 0) return false;     // see the right part of Figure 5
    det = sqrt(det);               //  we -use_fast_math in CUDA_NVRTC_OPTIONS 
    float u1, u2;                  // two roots(u parameter) 
    float t = ray.TMax, u, v;      // need solution for the smallest t > 0  
    if (c == 0) {                  // if c == 0, it is a trapezoid
        u1 = -a / b; u2 = -1;      // and there is only one root
    }
    else {                                   // (c != 0 in Stanford models) 
        u1 = (-b - CopySign(det, b)) / 2;    // numerically "stable" root 
        u2 = a / u1;                         // Viete's formula for u1*u2 
        u1 /= c;
    }
    if (0 <= u1 && u1 <= 1) {                // is it inside the patch? 
        float3 pa = lerp(q00, q10, u1);        //  point on edge e10 (Fig. 4) 
        float3 pb = lerp(e00, e11, u1);        // it is, actually, pb - pa 
        float3 n = cross(ray.Direction, pb);
        det = dot(n, n);
        n = cross(n, pa);
        float t1 = dot(n, pb);
        float v1 = dot(n, ray.Direction);          // no need to check t1 < t 
        if (t1 > 0 && 0 <= v1 && v1 <= det) {      // if t1 > ray.tmax, 
            t = t1 / det; u = u1; v = v1 / det;    // it will be rejected
        }                                          // in rtPotentialIntersection
    }
    if (0 <= u2 && u2 <= 1) {                  // it is slightly different,
        float3 pa = lerp(q00, q10, u2);        // since u1 might be good 
        float3 pb = lerp(e00, e11, u2);        // and we need 0 < t2 < t1 
        float3 n = cross(ray.Direction, pb);
        det = dot(n, n);
        n = cross(n, pa);
        float t2 = dot(n, pb) / det;
        float v2 = dot(n, ray.Direction);
        if (0 <= v2 && v2 <= det && t > t2 && t2 > 0) {
            t = t2; u = u2; v = v2 / det;
        }
    }

    if (t >= ray.TMin && t < ray.TMax)
    {
        intersectionPoint = ray.Origin + t * ray.Direction;
        return true;
    }
    else {
        return false;
    }
}

float PointDistanceToPlane(float3 p, Plane plane)
{
    float3 ray = p - plane.PointOnPlane;
    float distToPlane = dot(plane.Normal, ray);
    return distToPlane;
}

bool RayPlaneIntersection(Plane plane, Ray ray, out float3 intersectionPoint) 
{
    // Assuming float3s are all normalized
    float denom = dot(plane.Normal, ray.Direction) + 1e-06;
    float3 p0l0 = plane.PointOnPlane - ray.Origin;
    float t = dot(p0l0, plane.Normal) / denom;
    intersectionPoint = ray.Origin + ray.Direction * t;
    return t >= 0;
}

// Triangle intersection. Returns { t, u, v }
float3 RayTriangleIntersection(float3 ro, float3 rd, float3 v0, float3 v1, float3 v2)
{
    float3 v1v0 = v1 - v0;
    float3 v2v0 = v2 - v0;
    float3 rov0 = ro - v0;

    float3  n = cross(v1v0, v2v0);
    float3  q = cross(rov0, rd);
    float d = 1.0 / dot(rd, n);
    float u = d * dot(-q, v2v0);
    float v = d * dot(q, v1v0);
    float t = d * dot(-n, rov0);

    if (u < 0.0 || v < 0.0 || (u + v)>1.0) t = -1.0;

    return float3(t, u, v);
}

// Ray - cell intersection routine for height - fields given in  H. Ki and K. Oh. 
// Accurate per-pixel displacement mapping using a pyramid structure - http://ki-h.com/article/ipdm.html. 
// pages 55–62, New York, NY, USA, 2007. ACM. Was extended to 3D for DDM of resolution(Lx x Ly x Lz).
// Here e is a small constant.
// 1: s = floor(txLx, tyLy, tzLz) 
// 2: u = (sign(r) + 1.0) / 2 
// 3: w = ((sx + ux) / Lx, (sy + uy) / Ly, (sz + uz) / Lz) 
// 4: d = min((wx -tx) / rx, (wy -ty) / ry, (wz -tz) / rz) 
// 5: t = t + (d + e)r
//
float3 VoxelWallIntersection(float3 voxelUVW, uint3 voxelGridResolution, Ray ray)
{
    static const float Epsilon = 0.001;

    float3 s = floor(voxelUVW * voxelGridResolution);
    float3 u = (sign(ray.Direction) + 1.0) / 2.0;
    float3 w = (s + u) / voxelGridResolution;
    float3 a = (w - voxelUVW) / (ray.Direction/* + Epsilon*/); // Add e to prevent division by zero

    // Find minimum non-zero value in 'a'
    float d = a[0];
    for (uint i = 1; i < 3; ++i)
    {
        if ((a[i] < d && a[i] != 0) || d == 0)
        {
            d = a[i];
        }
    }

    return voxelUVW + ray.Direction * (d + Epsilon); // Add e to make sure we step into neighbor voxel
}

bool RaySphereIntersection(Sphere sphere, Ray ray, out float3 intersectionPoint) 
{
    float3 oc = ray.Origin - sphere.Center;
    float a = dot(ray.Direction, ray.Direction);
    float b = 2.0 * dot(oc, ray.Direction);
    float c = dot(oc, oc) - sphere.Radius * sphere.Radius;
    float discriminant = b * b - 4.0 * a * c;
    float discriminantSqrt = sqrt(discriminant);

    bool intersects = false;

    if (discriminant > 0.0)
    {
        float numerator = -b - discriminantSqrt;

        if (numerator <= 0.0)
        {
            numerator = -b + discriminantSqrt;
        }

        if (numerator > 0.0)
        {
            intersectionPoint = ray.Origin + ray.Direction * (numerator / (2.0 * a));
            intersects = true;
        }
    }

    return intersects;
}

bool IsPointInsideEllipse(float2 p, float2 ellipseCenter, float2 widthHeight)
{
    float2 half = widthHeight * 0.5;
    float2 delta = p - ellipseCenter;
    return ((delta.x * delta.x) / (half.x * half.x) + (delta.y * delta.y) / (half.y * half.y)) <= 1.0;
}

float3 SphericalToCartesian_ZUp(float theta, float phi)
{
    // theta - vertical angle
    // phi - horizontal angle
    
    // Assuming sphere of unit radius
    // and left-handed Cartesian coordinate system
    float sinTheta = sin(theta);
    float sinPhi = sin(phi);
    float cosTheta = cos(theta);
    float cosPhi = cos(phi);

    return float3(sinTheta * sinPhi, sinTheta * cosPhi, cosTheta);
}

#endif