#include "FlatLight.hpp"

#include <glm/geometric.hpp>

#include "../Foundation/Pi.hpp"

namespace PathFinder
{

    FlatLight::FlatLight(Type type)
        : mType{ type } {}

    void FlatLight::SetNormal(const glm::vec3& normal)
    {
        mNormal = glm::normalize(normal);
    }

    void FlatLight::SetPosition(const glm::vec3& position)
    {
        mPosition = position;
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

    void FlatLight::UpdateArea()
    {
        SetArea((mHeight / 2.0) * (mWidth / 2.0) * M_PI);
    }
    
}
