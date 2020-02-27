#pragma once

#include "Light.hpp"

#include <glm/vec3.hpp>

namespace PathFinder
{

    class SphericalLight : public Light
    {
    public:
        ~SphericalLight() = default;

        void SetPosition(const glm::vec3& position);
        void SetRadius(float radius);

    private:
        void UpdateArea();

        glm::vec3 mPosition;
        float mRadius = 1.0;

    public:
        inline const auto& Position() const { return mPosition; }
        inline const auto& Radius() const { return mRadius; }
    };

}