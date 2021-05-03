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
        void SetWidth(float width);
        void SetHeight(float height);
        void ConstructModelMatrix() override;

    private:
        void UpdateArea();

        Type mType;
        glm::vec3 mNormal;
        float mWidth = 0.0f;
        float mHeight = 0.0f;

    public:
        inline auto GetLightType() const { return mType; }
        inline const auto& GetNormal() const { return mNormal; }
        inline const auto& GetWidth() const { return mWidth; }
        inline const auto& GetHeight() const { return mHeight; }
    };

}
