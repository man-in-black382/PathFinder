#include "DiskLight.hpp"

#include "../Foundation/Pi.hpp"

#include <glm/geometric.hpp>

namespace PathFinder
{

    void DiskLight::SetNormal(const glm::vec3& normal)
    {
        mNormal = glm::normalize(normal);
    }

    void DiskLight::SetPosition(const glm::vec3& position)
    {
        mPosition = position;
    }

    void DiskLight::SetWidth(float width)
    {
        mWidth = width;
        UpdateArea();
    }

    void DiskLight::SetHeight(float height)
    {
        mHeight = height;
        UpdateArea();
    }

    void DiskLight::UpdateArea()
    {
        SetArea((mHeight / 2.0) * (mWidth / 2.0) * M_PI);
    }

}
