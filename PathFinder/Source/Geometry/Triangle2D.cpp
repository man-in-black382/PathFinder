#include "Triangle2D.hpp"
#include "AABB.hpp"

#include <glm/vec3.hpp>

namespace Geometry 
{

    float Triangle2D::GetArea() const 
    {
        glm::vec3 A(a, 1.0);
        glm::vec3 B(b, 1.0);
        glm::vec3 C(c, 1.0);

        return glm::length(glm::cross(C - A, B - A)) / 2.0f;
    }

}
