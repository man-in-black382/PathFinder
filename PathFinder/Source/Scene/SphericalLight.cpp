#include "SphericalLight.hpp"

#include <Foundation/Pi.hpp>

namespace PathFinder
{

    void SphericalLight::SetRadius(float radius)
    {
        mRadius = radius;
        UpdateArea();
    }

    void SphericalLight::ConstructModelMatrix()
    {
        mModelMatrix[0][0] = mRadius;
        mModelMatrix[1][1] = mRadius;
        mModelMatrix[2][2] = mRadius;
        mModelMatrix[3] = glm::vec4{ mPosition, 1.0f };
    }

    void SphericalLight::UpdateArea()
    {
        SetArea(4.0 * M_PI * mRadius * mRadius);
    }

}
