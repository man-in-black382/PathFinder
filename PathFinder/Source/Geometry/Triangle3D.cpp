#include "Triangle3D.hpp"
#include "AxisAlignedBox3D.hpp"

namespace Geometry {

    float Triangle3D::area() const {
        return glm::length(glm::cross(c - a, b - a)) / 2.0f;
    }

    glm::vec3 Triangle3D::normal() const {
        return glm::normalize(glm::cross(c - a, b - a));
    }

    AxisAlignedBox3D Triangle3D::boundingBox() const {
        glm::vec3 min = glm::min(p1, p2);
        min = glm::min(min, p3);

        glm::vec3 max = glm::max(p1, p2);
        max = glm::max(min, p3);

        return { min, max };
    }

    std::array<Triangle3D, 4> Triangle3D::split() const {
        glm::vec3 halfAB = (a + b) / 2.0f;
        glm::vec3 halfAC = (a + c) / 2.0f;
        glm::vec3 halfBC = (b + c) / 2.0f;

        return {
                Triangle3D(a, halfAB, halfAC),
                Triangle3D(halfAB, b, halfBC),
                Triangle3D(halfBC, c, halfAC),
                Triangle3D(halfAB, halfAC, halfBC)
        };
    }

}
