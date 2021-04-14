#pragma once

#include "Interval.hpp"
#include "AABB.hpp"
#include "Ray3D.hpp"
#include "Triangle2D.hpp"
#include "Triangle3D.hpp"
#include "Sphere.hpp"
#include "Plane.hpp"

#include <glm/vec3.hpp>

namespace Geometry 
{

    namespace Collision
    {
        glm::vec3 Barycentric(const glm::vec3& point, const Triangle3D& triangle);
        glm::vec3 Project(const glm::vec3& first, const glm::vec3& second);
        bool TriangleAABB(const Triangle3D& t, const AABB& a);
        bool RayAABB(const Ray3D& ray, const AABB& aabb, float& distance);
        bool RayParallelogram(const Ray3D& ray, const Parallelogram3D& parallelogram, float& distance);
        bool RayPlane(const Ray3D& ray, const Plane& plane, float& distance);
        bool RayTriangle(const Ray3D& ray, const Triangle3D& triangle, float& distance);
        bool AABBContainsPoint(const AABB& aabb, const glm::vec3& point);
        bool AABBContainsTriangle(const AABB& aabb, const Triangle3D& triangle);
        bool AABBContainsAABB(const AABB& aabb1, const AABB& aabb2);
    }

}
