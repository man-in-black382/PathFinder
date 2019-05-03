#include "Triangle2D.hpp"
#include "AxisAlignedBox3D.hpp"

#include <glm/vec3.hpp>

namespace Geometry {

    float Triangle2D::area() const {
        glm::vec3 A(a, 1.0);
        glm::vec3 B(b, 1.0);
        glm::vec3 C(c, 1.0);

        return glm::length(glm::cross(C - A, B - A)) / 2.0f;
    }

    std::array<Triangle2D, 4> Triangle2D::split() const {
        glm::vec2 halfAB = (a + b) / 2.0f;
        glm::vec2 halfAC = (a + c) / 2.0f;
        glm::vec2 halfBC = (b + c) / 2.0f;

        return {
                Triangle2D(a, halfAB, halfAC),
                Triangle2D(halfAB, b, halfBC),
                Triangle2D(halfBC, c, halfAC),
                Triangle2D(halfAB, halfAC, halfBC)
        };
    }

    Rect2D Triangle2D::boundingRect() const {
        glm::vec2 min = glm::min(p1, p2);
        min = glm::min(min, p3);

        glm::vec2 max = glm::max(p1, p2);
        max = glm::max(min, p3);

        glm::vec2 delta = max - min;

        return {min, {delta.x, delta.y}};
    }

}
