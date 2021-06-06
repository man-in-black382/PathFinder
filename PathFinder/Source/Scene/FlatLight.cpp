#include "FlatLight.hpp"

#include <glm/geometric.hpp>
#include <glm/gtx/transform.hpp>

#include <Foundation/Pi.hpp>

namespace PathFinder
{

    FlatLight::FlatLight(Type type)
        : mType{ type } 
    {
        ConstructModelMatrix();
    }

    void FlatLight::SetRotation(const glm::quat& rotation)
    {
        mRotation = rotation;
    }

    void FlatLight::SetWidth(float width)
    {
        mWidth = width;
        UpdateArea();
    }

    void FlatLight::SetHeight(float height)
    {
        mHeight = height;
        UpdateArea();
    }

    void FlatLight::ConstructModelMatrix()
    {
        glm::mat4 translationMat = glm::translate(mPosition);
        glm::mat4 rotationMat = glm::mat4_cast(mRotation);
        glm::mat4 scaleMat = glm::scale(glm::vec3{ mWidth, mHeight, 1.0f });

        mModelMatrix = translationMat * rotationMat * scaleMat;
    }

    void FlatLight::UpdateArea()
    {
        switch (mType)
        {
        case Type::Disk: SetArea(mHeight * 0.5 * mWidth * 0.5 * M_PI); break;
        case Type::Rectangle: SetArea(mHeight * mWidth); break;
        default: break;
        }
    }

}
