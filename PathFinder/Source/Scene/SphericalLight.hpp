#pragma once

#include "Light.hpp"

#include <glm/vec3.hpp>

namespace PathFinder
{

    class SphericalLight : public Light
    {
    public:
        SphericalLight();
        ~SphericalLight() = default;

        void SetRadius(float radius);
        void ConstructModelMatrix() override;

    private:
        void UpdateArea();

        float mRadius = 1.0;

    public:
        inline const auto& GetRadius() const { return mRadius; }
    };

}