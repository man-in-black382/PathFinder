#pragma once

#include "Light.hpp"

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

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

        void SetRotation(const glm::quat& rotation);
        void SetWidth(float width);
        void SetHeight(float height);
        void ConstructModelMatrix() override;

    private:
        void UpdateArea();

        Type mType;
        glm::quat mRotation = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        float mWidth = 1.0f;
        float mHeight = 1.0f;

    public:
        inline Type GetLightType() const { return mType; }
        inline const glm::quat& GetRotation() const { return mRotation; }
        inline float GetWidth() const { return mWidth; }
        inline float GetHeight() const { return mHeight; }
    };

}
