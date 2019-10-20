#include "Transformation.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Geometry {

    Transformation::Transformation() : Scale(glm::one<glm::vec3>()), Translation(glm::zero<glm::vec3>()), Rotation(glm::quat()) {}

    Transformation::Transformation(const glm::mat4 &matrix) 
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, Scale, Rotation, Translation, skew, perspective);
    }

    Transformation::Transformation(glm::vec3 scale, glm::vec3 translation, glm::quat rotation)
            : Scale(scale), Translation(translation), Rotation(rotation) {}

    Transformation Transformation::CombinedWith(const Transformation &other) const  
    { 
        return Transformation(other.ModelMatrix() * ModelMatrix()); 
    }

    glm::mat4 Transformation::ModelMatrix() const 
    {
        return glm::translate(Translation) * glm::mat4_cast(Rotation) * glm::scale(Scale);
    }

    glm::mat4 Transformation::ScaleMatrix() const
    {
        return glm::scale(Scale);
    }

    glm::mat4 Transformation::RotationMatrix() const 
    {
        return glm::mat4_cast(Rotation);
    }

    glm::mat4 Transformation::TranslationMatrix() const 
    {
        return glm::translate(Translation);
    }

    glm::mat4 Transformation::NormalMatrix() const
    {
        return glm::transpose(glm::inverse(ModelMatrix()));
    }

    glm::mat4 Transformation::InverseScaleMatrix() const 
    {
        return glm::inverse(glm::scale(Scale));
    }

    glm::mat4 Transformation::InverseRotationMatrix() const
    {
        return glm::inverse(glm::mat4_cast(Rotation));
    }

    glm::mat4 Transformation::InverseTranslationMatrix() const
    {
        return glm::inverse(glm::translate(Translation));
    }

}
