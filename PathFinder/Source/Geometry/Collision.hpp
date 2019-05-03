#pragma once

#include "Interval.hpp"
#include "AxisAlignedBox3D.hpp"
#include "Ray3D.hpp"
#include "Triangle2D.hpp"
#include "Triangle3D.hpp"
#include "Sphere.hpp"
#include "Plane.hpp"

#include <glm/vec3.hpp>

namespace Geometry {

    class Collision {
    private:
        static Interval GetInterval(const Triangle3D &triangle, const glm::vec3 &axis);

        static Interval GetInterval(const AxisAlignedBox3D &aabb, const glm::vec3 &axis);

        static bool OverlapOnAxis(const AxisAlignedBox3D &aabb, const Triangle3D &triangle, const glm::vec3 &axis);

    public:
        static glm::vec3 Barycentric(const glm::vec3 &point, const Triangle3D &triangle);

        static glm::vec3 Project(const glm::vec3 &first, const glm::vec3 &second);

        static bool TriangleAABB(const Triangle3D &t, const AxisAlignedBox3D &a);

        static bool RayAABB(const Ray3D &ray, const AxisAlignedBox3D &aabb, float &distance);

        static bool RayParallelogram(const Ray3D &ray, const Parallelogram3D &parallelogram, float &distance);

        static bool RayPlane(const Ray3D &ray, const Plane &plane, float &distance);

        static bool RayTriangle(const Ray3D &ray, const Triangle3D &triangle, float &distance);
    };

}
