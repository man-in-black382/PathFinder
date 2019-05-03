//
//  Transform.cpp
#include "Transformation.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Geometry {

    Transformation::Transformation()
            : scale(glm::one<glm::vec3>()), translation(glm::zero<glm::vec3>()), rotation(glm::quat()) {}

    Transformation::Transformation(const glm::mat4 &matrix) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, scale, rotation, translation, skew, perspective);
    }

    Transformation::Transformation(glm::vec3 scale, glm::vec3 translation, glm::quat rotation)
            : scale(scale), translation(translation), rotation(rotation) {}

    Transformation Transformation::combinedWith(const Transformation &other) const {
        return Transformation(other.modelMatrix() * modelMatrix());
    }

    glm::mat4 Transformation::modelMatrix() const {
        return glm::translate(translation) * glm::mat4_cast(rotation) * glm::scale(scale);
    }

    glm::mat4 Transformation::scaleMatrix() const {
        return glm::scale(scale);
    }

    glm::mat4 Transformation::rotationMatrix() const {
        return glm::mat4_cast(rotation);
    }

    glm::mat4 Transformation::translationMatrix() const {
        return glm::translate(translation);
    }

    glm::mat4 Transformation::normalMatrix() const {
        return glm::transpose(glm::inverse(modelMatrix()));
    }

    glm::mat4 Transformation::inverseScaleMatrix() const {
        return glm::inverse(glm::scale(scale));
    }

    glm::mat4 Transformation::inverseRotationMatrix() const {
        return glm::inverse(glm::mat4_cast(rotation));
    }

    glm::mat4 Transformation::inverseTranslationMatrix() const {
        return glm::inverse(glm::translate(translation));
    }

}
