#include "Parallelogram3D.hpp"
#include <glm/geometric.hpp>

namespace Geometry {

    Parallelogram3D::Parallelogram3D(const glm::vec3 &corner,
            const glm::vec3 &sideVector1,
            const glm::vec3 &sideVector2)
            :
            corner(corner),
            side1(sideVector1),
            side2(sideVector2) {
    }

    glm::vec3 Parallelogram3D::normal() const {
        return glm::normalize(glm::cross(side1, side2));
    }

    Parallelogram3D Parallelogram3D::transformedBy(const Transformation &t) const {
        return transformedBy(t.modelMatrix());
    }

    Parallelogram3D Parallelogram3D::transformedBy(const glm::mat4 &mat) const {
        glm::vec3 newCorner = mat * glm::vec4(corner, 1.0);
        // w = 0 to exclude translation from direction vectors
        glm::vec3 newSide1 = mat * glm::vec4(side1, 0.0);
        glm::vec3 newSide2 = mat * glm::vec4(side2, 0.0);
        return {newCorner, newSide1, newSide2};
    }

}
