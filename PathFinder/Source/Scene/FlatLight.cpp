#include "FlatLight.hpp"

#include <glm/geometric.hpp>
#include <glm/gtx/transform.hpp>

#include <Foundation/Pi.hpp>

namespace PathFinder
{

    FlatLight::FlatLight(Type type)
        : mType{ type } {}

    void FlatLight::SetNormal(const glm::vec3& normal)
    {
        mNormal = glm::normalize(normal);
        ConstructModelMatrix();
    }

    void FlatLight::SetPosition(const glm::vec3& position)
    {
        mPosition = position;
        ConstructModelMatrix();
    }

    void FlatLight::SetWidth(float width)
    {
        mWidth = width;
        UpdateArea();
        ConstructModelMatrix();
    }

    void FlatLight::SetHeight(float height)
    {
        mHeight = height;
        UpdateArea();
        ConstructModelMatrix();
    }

    void FlatLight::UpdateArea()
    {
        SetArea((mHeight / 2.0) * (mWidth / 2.0) * M_PI);
    }
    
    void FlatLight::ConstructModelMatrix()
    {
        glm::vec3 UpY{ 0.0, 1.0, 0.0 };
        glm::vec3 UpZ{ 0.0, 0.0, 1.0 };

        glm::vec3 up = glm::abs(glm::dot(mNormal, UpY)) < 0.999 ? UpY : UpZ;
        mModelMatrix = glm::translate(mPosition) * glm::lookAt(mPosition, mPosition + mNormal, up) * glm::scale(glm::vec3{ mWidth, mHeight, 1.0f });
    }

}
