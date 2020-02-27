#include "SphericalLight.hpp"

#include "../Foundation/Pi.hpp"

namespace PathFinder
{

    void SphericalLight::SetPosition(const glm::vec3& position)
    {
        mPosition = position;
    }

    void SphericalLight::SetRadius(float radius)
    {
        mRadius = radius;
        UpdateArea();
    }

    void SphericalLight::UpdateArea()
    {
        SetArea(4.0 * M_PI * mRadius * mRadius);
    }

}
