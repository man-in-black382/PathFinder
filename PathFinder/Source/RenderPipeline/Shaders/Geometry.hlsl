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

bool IntersectPlane(float3 pointOnPlane, float3 planeNormal, float3 rayOrigin, float3 rayDirection, out float t) 
{
    // Assuming float3s are all normalized
    float denom = dot(planeNormal, rayDirection);
    float3 p0l0 = pointOnPlane - rayOrigin;
    t = dot(p0l0, planeNormal) / denom;
    return (t >= 0);
}

// Triangle intersection. Returns { t, u, v }
float3 triIntersect(float3 ro, float3 rd, float3 v0, float3 v1, float3 v2)
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

//// What is the x,y,z position of a point at params u and v?
//float3 BilinearPatch::SrfEval(float u, float v)
//{
//    float3 respt;
//
//    respt.x(((1.0 - u) * (1.0 - v) * P00.x() +
//
//        (1.0 - u) * v * P01.x() +
//
//        u * (1.0 - v) * P10.x() +
//
//        u * v * P11.x()));
//
//    respt.y(((1.0 - u) * (1.0 - v) * P00.y() +
//
//        (1.0 - u) * v * P01.y() +
//
//        u * (1.0 - v) * P10.y() +
//
//        u * v * P11.y()));
//
//    respt.z(((1.0 - u) * (1.0 - v) * P00.z() +
//
//        (1.0 - u) * v * P01.z() +
//
//        u * (1.0 - v) * P10.z() +
//
//        u * v * P11.z()));
//
//    return respt;
//
//}
//
//
//
////+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+
//
//// Find tangent (du)
//
////+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+
//
//float3 BilinearPatch::TanU(float v)
//{
//    float3 tanu;
//    tanu.x((1.0 - v) * (P10.x() - P00.x()) + v * (P11.x() - P01.x()));
//    tanu.y((1.0 - v) * (P10.y() - P00.y()) + v * (P11.y() - P01.y()));
//    tanu.z((1.0 - v) * (P10.z() - P00.z()) + v * (P11.z() - P01.z()));
//    return tanu;
//}
//
//
//
////+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+
//
//// Find tanget (dv)
//
////+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+
//
//float3 BilinearPatch::TanV(float u)
//
//{
//    float3 tanv;
//    tanv.x((1.0 - u) * (P01.x() - P00.x()) + u * (P11.x() - P10.x()));
//    tanv.y((1.0 - u) * (P01.y() - P00.y()) + u * (P11.y() - P10.y()));
//    tanv.z((1.0 - u) * (P01.z() - P00.z()) + u * (P11.z() - P10.z()));
//    return tanv;
//}
//
////+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+
//
//// Find the normal of the patch
//
////+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+
//
//float3 BilinearPatch::Normal(float u, float v)
//
//{
//    float3 tanu, tanv;
//    tanu = TanU(v);
//    tanv = TanV(u);
//    return tanu.cross(tanv);
//}
//
////choose between the best denominator to avoid singularities
////and to get the most accurate root possible
//inline float getu(float v, float M1, float M2, float J1, float J2,
//    float K1, float K2, float R1, float R2)
//
//{
//
//    float denom = (v * (M1 - M2) + J1 - J2);
//    float d2 = (v * M1 + J1);
//
//    if (fabs(denom) > fabs(d2)) // which denominator is bigger
//    {
//        return (v * (K2 - K1) + R2 - R1) / denom;
//    }
//
//    return -(v * K1 + R1) / d2;
//}
//
//
//
//// compute t with the best accuracy by using the component
//// of the direction that is largest
//
//float computet(float3 dir, float3 orig, float3 srfpos)
//
//{
//
//    // if x is bigger than y and z
//
//    if (fabs(dir.x()) >= fabs(dir.y()) && fabs(dir.x()) >= fabs(dir.z()))
//
//        return (srfpos.x() - orig.x()) / dir.x();
//
//    // if y is bigger than x and z
//
//    else if (fabs(dir.y()) >= fabs(dir.z())) // && fabs(dir.y()) >= fabs(dir.x()))
//
//        return (srfpos.y() - orig.y()) / dir.y();
//
//    // otherwise x isn't bigger than both and y isn't bigger than both
//
//    else  //if(fabs(dir.z()) >= fabs(dir.x()) && fabs(dir.z()) >= fabs(dir.y()))
//
//        return (srfpos.z() - orig.z()) / dir.z();
//
//}
//
//





////+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+
//
////             RayPatchIntersection
//
//// intersect rays of the form p = r + t q where t is the parameter
//
//// to solve for. With the patch pointed to by *this
//
//// for valid intersections:
//
//// place the u,v intersection point in uv[0] and uv[1] respectively.
//
//// place the t value in uv[2]
//
//// return true to this function
//
//// for invalid intersections - simply return false uv values can be 
//
//// anything
//
////+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+
//
//bool BilinearPatch::RayPatchIntersection(float3 r, float3 q, float3& uv)
//
//{
//    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    // Equation of the patch:
//
//    // P(u, v) = (1-u)(1-v)P00 + (1-u)vP01 + u(1-v)P10 + uvP11
//
//    // Equation of the ray:
//
//    // R(t) = r + tq
//
//    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    float3 pos1, pos2; //float3 pos = ro + t*rd;
//    int num_sol; // number of solutions to the quadratic
//    float vsol[2]; // the two roots from quadraticroot
//    float t2, u; // the t values of the two roots
//
//    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    // Variables for substitition
//
//    // a = P11 - P10 - P01 + P00
//
//    // b = P10 - P00
//
//    // c = P01 - P00
//
//    // d = P00  (d is shown below in the #ifdef raypatch area)
//
//    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~  
//
//    // Find a w.r.t. x, y, z
//    float ax = P11.x() - P10.x() - P01.x() + P00.x();
//    float ay = P11.y() - P10.y() - P01.y() + P00.y();
//    float az = P11.z() - P10.z() - P01.z() + P00.z();
//
//    // Find b w.r.t. x, y, z
//    float bx = P10.x() - P00.x();
//    float by = P10.y() - P00.y();
//    float bz = P10.z() - P00.z();
//
//    // Find c w.r.t. x, y, z
//    float cx = P01.x() - P00.x();
//    float cy = P01.y() - P00.y();
//    float cz = P01.z() - P00.z();
//
//    float rx = r.x();
//    float ry = r.y();
//    float rz = r.z();
//
//    // Retrieve the xyz of the q part of ray
//    float qx = q.x();
//    float qy = q.y();
//    float qz = q.z();
//
//    // Find d w.r.t. x, y, z - subtracting r just after  
//    float dx = P00.x() - r.x();
//    float dy = P00.y() - r.y();
//    float dz = P00.z() - r.z();
//
//
//    // Find A1 and A2
//    float A1 = ax * qz - az * qx;
//    float A2 = ay * qz - az * qy;
//
//
//    // Find B1 and B2
//    float B1 = bx * qz - bz * qx;
//    float B2 = by * qz - bz * qy;
//
//    // Find C1 and C2
//    float C1 = cx * qz - cz * qx;
//    float C2 = cy * qz - cz * qy;
//
//    // Find D1 and D2
//    float D1 = dx * qz - dz * qx;
//    float D2 = dy * qz - dz * qy;
//
//    float3 dir(qx, qy, qz), orig(rx, ry, rz);
//    float A = A2 * C1 - A1 * C2;
//    float B = A2 * D1 - A1 * D2 + B2 * C1 - B1 * C2;
//    float C = B2 * D1 - B1 * D2;
//
//    uv.x(-2); uv.y(-2); uv.z(-2);
//
//    num_sol = QuadraticRoot(A, B, C, -ray_epsilon, 1 + ray_epsilon, vsol);
//
//    switch (num_sol)
//    {
//    case 0:
//        return false; // no solutions found
//
//    case 1:
//        uv.y(vsol[0]);
//        uv.x(getu(uv.y(), A2, A1, B2, B1, C2, C1, D2, D1));
//        pos1 = SrfEval(uv.x(), uv.y());
//        uv.z(computet(dir, orig, pos1));
//
//        if (uv.x() < 1 + ray_epsilon && uv.x() > -ray_epsilon && uv.z() > 0)//vars okay?
//            return true;
//        else
//            return false; // no other soln - so ret false
//
//    case 2: // two solutions found
//        uv.y(vsol[0]);
//        uv.x(getu(uv.y(), A2, A1, B2, B1, C2, C1, D2, D1));
//        pos1 = SrfEval(uv.x(), uv.y());
//        uv.z(computet(dir, orig, pos1));
//
//        if (uv.x() < 1 + ray_epsilon && uv.x() > -ray_epsilon && uv.z() > 0)
//        {
//            u = getu(vsol[1], A2, A1, B2, B1, C2, C1, D2, D1);
//
//            if (u < 1 + ray_epsilon && u > ray_epsilon)
//            {
//                pos2 = SrfEval(u, vsol[1]);
//                t2 = computet(dir, orig, pos2);
//
//                if (t2 < 0 || uv.z() < t2) // t2 is bad or t1 is better
//                    return true;
//
//                // other wise both t2 > 0 and t2 < t1
//                uv.y(vsol[1]);
//                uv.x(u);
//                uv.z(t2);
//                return true;
//
//            }
//
//            return true; // u2 is bad but u1 vars are still okay
//        }
//
//        else // doesn't fit in the root - try other one
//        {
//            uv.y(vsol[1]);
//            uv.x(getu(vsol[1], A2, A1, B2, B1, C2, C1, D2, D1));
//            pos1 = SrfEval(uv.x(), uv.y());
//            uv.z(computet(dir, orig, pos1));
//
//            if (uv.x() < 1 + ray_epsilon && uv.x() > -ray_epsilon && uv.z() > 0)
//                return true;
//            else
//                return false;
//
//        }
//
//        break;
//    }
//
//    return false;
//}
//
//// a x ^2 + b x + c = 0
//// in this case, the root must be between min and max
//// it returns the # of solutions found
//// x = [ -b +/- sqrt(b*b - 4 *a*c) ] / 2a
//// or x = 2c / [-b +/- sqrt(b*b-4*a*c)]
//int QuadraticRoot(float a, float b, float c,
//    float min, float max, float* u)
//{
//    u[0] = u[1] = min - min; // make it lower than min
//
//    if (a == 0.0) // then its close to 0
//    {
//        if (b != 0.0) // not close to 0
//        {
//            u[0] = -c / b;
//
//            if (u[0] > min && u[0] < max) //its in the interval
//                return 1; //1 soln found
//            else  //its not in the interval
//                return 0;
//        }
//
//        else
//            return 0;
//    }
//
//    float d = b * b - 4 * a * c; //discriminant
//
//    if (d <= 0.0) // single or no root
//    {
//        if (d == 0.0) // close to 0
//        {
//            u[0] = -b / a;
//
//            if (u[0] > min && u[0] < max) // its in the interval
//                return 1;
//            else //its not in the interval
//                return 0;
//
//        }
//
//        else // no root d must be below 0
//            return 0;
//    }
//
//    float q = -0.5 * (b + CopySign(sqrt(d), b));
//
//    u[0] = c / q;
//    u[1] = q / a;
//
//    if ((u[0] > min && u[0] < max)
//        && (u[1] > min && u[1] < max))
//        return 2;
//
//    else if (u[0] > min && u[0] < max) //then one wasn't in interval
//        return 1;
//
//    else if (u[1] > min && u[1] < max)
//    {  // make it easier, make u[0] be the valid one always
//        float dummy;
//        dummy = u[0];
//
//        u[0] = u[1];
//        u[1] = dummy; // just in case somebody wants to check it
//
//        return 1;
//
//    }
//
//    return 0;
//}
