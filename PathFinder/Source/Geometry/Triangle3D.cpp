#include "Triangle3D.hpp"
#include "AABB.hpp"

namespace Geometry
{

    float Triangle3D::GetArea() const 
    {
        return glm::length(glm::cross(c - a, b - a)) / 2.0f;
    }

    glm::vec3 Triangle3D::GetNormal() const 
    {
        return glm::normalize(glm::cross(c - a, b - a));
    }

}
