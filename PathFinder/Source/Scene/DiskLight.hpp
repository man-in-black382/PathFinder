#pragma once

#include "Light.hpp"

#include <glm/vec3.hpp>

namespace PathFinder 
{

    class DiskLight : public Light
    {
    public:
        ~DiskLight() = default;

        void SetNormal(const glm::vec3& normal);
        void SetPosition(const glm::vec3& position);
        void SetWidth(float width);
        void SetHeight(float height);

    private:
        void UpdateArea();

        glm::vec3 mNormal;
        glm::vec3 mPosition;
        float mWidth = 0.0f;
        float mHeight = 0.0f;

    public:
        inline const auto& Normal() const { return mNormal; }
        inline const auto& Position() const { return mPosition; }
        inline const auto& Width() const { return mWidth; }
        inline const auto& Height() const { return mHeight; }
    };

}
