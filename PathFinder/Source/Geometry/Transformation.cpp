#include "Transformation.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Geometry {

    Transformation::Transformation()
        : mScale(glm::one<glm::vec3>()),
        mTranslation(glm::zero<glm::vec3>()),
        mRotation(glm::quat()) {}

    Transformation::Transformation(const glm::mat4& matrix)
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, mScale, mRotation, mTranslation, skew, perspective);
    }

    Transformation::Transformation(const glm::vec3& scale, const glm::vec3& translation, const glm::quat& rotation)
        : mScale(scale), mTranslation(translation), mRotation(rotation) {}

    Transformation Transformation::CombinedWith(const Transformation& other) const
    {
        return Transformation(other.GetMatrix() * GetMatrix());
    }

    void Transformation::SetScale(const glm::vec3& scale)
    {
        mScale = scale;
        mIsDirty = true;
    }

    void Transformation::SetTranslation(const glm::vec3& translation)
    {
        mTranslation = translation;
        mIsDirty = true;
    }

    void Transformation::SetRotation(const glm::quat& rotation)
    {
        mRotation = rotation;
        mIsDirty = true;
    }

    const glm::mat4& Transformation::GetMatrix() const
    {
        if (mIsDirty)
        {
            mMatrix = glm::translate(mTranslation) * glm::mat4_cast(mRotation) * glm::scale(mScale);
            mIsDirty = false;
        }

        return mMatrix;
    }

    glm::mat4 Transformation::GetNormalMatrix() const
    {
        return glm::transpose(glm::inverse(GetMatrix()));
    }

}
