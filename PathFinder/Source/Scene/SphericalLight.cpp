#include "SphericalLight.hpp"

#include <Foundation/Pi.hpp>

namespace PathFinder
{

    void SphericalLight::SetPosition(const glm::vec3& position)
    {
        mPosition = position;
        mModelMatrix[3] = glm::vec4{ position, 1.0f };
    }

    void SphericalLight::SetRadius(float radius)
    {
        mRadius = radius;
        mModelMatrix[0][0] = radius;
        mModelMatrix[1][1] = radius;
        mModelMatrix[2][2] = radius;
        UpdateArea();
    }

    void SphericalLight::UpdateArea()
    {
        SetArea(4.0 * M_PI * mRadius * mRadius);
    }

}
