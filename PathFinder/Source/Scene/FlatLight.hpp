#pragma once

#include "Light.hpp"

#include <glm/vec3.hpp>

namespace PathFinder 
{

    class FlatLight : public Light
    {
    public:
        enum class Type
        {
            Disk, Rectangle
        };

        FlatLight(Type type);
        ~FlatLight() = default;

        void SetNormal(const glm::vec3& normal);
        void SetPosition(const glm::vec3& position);
        void SetWidth(float width);
        void SetHeight(float height);

    private:
        void UpdateArea();
        void ConstructModelMatrix();

        Type mType;
        glm::vec3 mNormal;
        glm::vec3 mPosition;
        float mWidth = 0.0f;
        float mHeight = 0.0f;

    public:
        inline auto LightType() const { return mType; }
        inline const auto& Normal() const { return mNormal; }
        inline const auto& Position() const { return mPosition; }
        inline const auto& Width() const { return mWidth; }
        inline const auto& Height() const { return mHeight; }
    };

}
