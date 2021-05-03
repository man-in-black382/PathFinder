#pragma once

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <bitsery/bitsery.h>
#include <Utility/SerializationAdapters.hpp>

namespace Geometry 
{

    class Transformation
    {
    public:
        Transformation();
        Transformation(const glm::mat4& matrix);
        Transformation(const glm::vec3& scale, const glm::vec3& translation, const glm::quat& rotation);
        Transformation CombinedWith(const Transformation& other) const;

        void SetScale(const glm::vec3& scale);
        void SetTranslation(const glm::vec3& translation);
        void SetRotation(const glm::quat& rotation);

        const glm::mat4& GetMatrix() const;
        glm::mat4 GetNormalMatrix() const;

    private:
        friend bitsery::Access;

        template <typename S>
        void serialize(S& s)
        {
            s.object(mScale);
            s.object(mTranslation);
            s.object(mRotation);
        }

        mutable bool mIsDirty = true;
        mutable glm::mat4 mMatrix;
        glm::vec3 mScale;
        glm::vec3 mTranslation;
        glm::quat mRotation;

    public:
        inline const glm::vec3& GetScale() const { return mScale; }
        inline const glm::vec3& GetTranslation() const { return mTranslation; }
        inline const glm::quat& GetRotation() const { return mRotation; }
    };

}
