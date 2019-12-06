#include "Utils.hlsl"

struct Ray
{
    float3 Origin;
    float3 Direction;
    float TMin;
    float TMax;
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

float3 InterpolatePatch(BilinearPatch patch, float2 uv)
{
    return (1.0 - uv.x) * (1.0 - uv.y) * patch.Q00 +
        (1.0 - uv.x) * uv.y * patch.Q01 +
        uv.x * (1.0 - uv.y) * patch.Q10 +
        uv.x * uv.y * patch.Q11;
}

// Ray Tracing Gems: A Geometric Approach to Ray/Bilinear Patch Intersections
//
bool IntersectPatch(BilinearPatch patch, Ray ray, out float3 intersectionPoint)
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

    float3 qn = normalize(cross(e00, e10));

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
    float t = FloatMax, u, v;      // need solution for the smallest t > 0 
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

    if (t > ray.TMin && t < ray.TMax)
    {
        intersectionPoint = ray.Origin + t * ray.Direction;
        return true;
    }
    else {
        return false;
    }
}