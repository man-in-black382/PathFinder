#include "Sphere.hpp"

#include <glm/gtx/norm.hpp>

namespace Geometry {

    Sphere::Sphere(const glm::vec3 &c, float r) : center(c), radius(r) {}

    bool Sphere::contains(const glm::vec3 &point) const {
        float radius2 = radius * radius;
        float length2 = glm::length2(point - center);

        return length2 <= radius2;
    }

    bool Sphere::contains(const Triangle3D &triangle) const {
        return contains(triangle.a) && contains(triangle.b) && contains(triangle.c);
    }

}
